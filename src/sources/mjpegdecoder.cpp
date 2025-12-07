#include "includes/mjpegdecoder.hpp"
#include <QBuffer>
#include <QMutex>
#include <QMutexLocker>
#include <QDebug>

const QByteArray MjpegDecoder::BOUNDARY_START = QByteArray("--");
const QByteArray MjpegDecoder::JPEG_START = QByteArray("\xff\xd8");
const QByteArray MjpegDecoder::JPEG_END = QByteArray("\xff\xd9");

// MjpegImageProvider implementation
MjpegImageProvider::MjpegImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // Create a default black image
    m_image = QImage(640, 480, QImage::Format_RGB888);
    m_image.fill(Qt::black);
}

QImage MjpegImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    Q_UNUSED(id);
    Q_UNUSED(requestedSize);

    QMutexLocker locker(&m_mutex);

    if (size) {
        *size = m_image.size();
    }

    return m_image;
}

void MjpegImageProvider::updateImage(const QImage &image)
{
    QMutexLocker locker(&m_mutex);
    m_image = image;
}

// MjpegDecoder implementation
MjpegDecoder::MjpegDecoder(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_reply(nullptr)
    , m_connected(false)
    , m_fps(0)
    , m_frameCount(0)
    , m_imageProvider(new MjpegImageProvider())
{
    m_fpsTimer = new QTimer(this);
    connect(m_fpsTimer, &QTimer::timeout, this, &MjpegDecoder::updateFpsCounter);
    m_fpsTimer->start(1000); // Update FPS every second
}

MjpegDecoder::~MjpegDecoder()
{
    stop();
    delete m_imageProvider;
}

void MjpegDecoder::setUrl(const QString &url)
{
    if (m_url != url) {
        m_url = url;
        emit urlChanged();
    }
}

void MjpegDecoder::start()
{
    if (m_url.isEmpty()) {
        emit errorOccurred("URL is empty");
        return;
    }

    stop(); // Stop any existing connection

    qDebug() << "Starting MJPEG stream from:" << m_url;

    QNetworkRequest request(m_url);
    request.setRawHeader("User-Agent", "MjpegDecoder/1.0");

    m_reply = m_networkManager->get(request);

    connect(m_reply, &QNetworkReply::readyRead, this, &MjpegDecoder::onReadyRead);
    connect(m_reply, &QNetworkReply::finished, this, &MjpegDecoder::onFinished);
    connect(m_reply, &QNetworkReply::errorOccurred, this, &MjpegDecoder::onError);
}

void MjpegDecoder::stop()
{
    if (m_reply) {
        m_reply->abort();
        m_reply->deleteLater();
        m_reply = nullptr;
    }

    m_buffer.clear();
    setConnected(false);
}

void MjpegDecoder::reconnect()
{
    qDebug() << "Reconnecting to MJPEG stream...";
    start();
}

void MjpegDecoder::onReadyRead()
{
    if (!m_reply) return;

    m_buffer.append(m_reply->readAll());
    processBuffer();
}

void MjpegDecoder::onFinished()
{
    qDebug() << "MJPEG stream finished";
    setConnected(false);

    if (m_reply && m_reply->error() == QNetworkReply::NoError) {
        // Stream ended normally, try to reconnect
        QTimer::singleShot(1000, this, &MjpegDecoder::reconnect);
    }
}

void MjpegDecoder::onError(QNetworkReply::NetworkError error)
{
    if (!m_reply) return;

    QString errorString = m_reply->errorString();
    qDebug() << "MJPEG decoder error:" << error << errorString;

    emit errorOccurred(errorString);
    setConnected(false);

    // Try to reconnect after 3 seconds
    QTimer::singleShot(3000, this, &MjpegDecoder::reconnect);
}

void MjpegDecoder::processBuffer()
{
    while (true) {
        // Find JPEG start marker
        int jpegStart = m_buffer.indexOf(JPEG_START);
        if (jpegStart == -1) {
            // No JPEG start found, keep last 2 bytes in case boundary is split
            if (m_buffer.size() > 2) {
                m_buffer = m_buffer.right(2);
            }
            break;
        }

        // Find JPEG end marker
        int jpegEnd = m_buffer.indexOf(JPEG_END, jpegStart + 2);
        if (jpegEnd == -1) {
            // No complete JPEG yet, remove data before jpegStart
            m_buffer = m_buffer.mid(jpegStart);
            break;
        }

        // Extract complete JPEG (including end marker)
        jpegEnd += 2; // Include the end marker
        QByteArray jpegData = m_buffer.mid(jpegStart, jpegEnd - jpegStart);

        // Remove processed data from buffer
        m_buffer = m_buffer.mid(jpegEnd);

        // Decode JPEG
        QImage image = decodeJpeg(jpegData);
        if (!image.isNull()) {
            m_imageProvider->updateImage(image);
            emit imageUpdated();
            m_frameCount++;

            if (!m_connected) {
                setConnected(true);
            }
        }
    }
}

QImage MjpegDecoder::decodeJpeg(const QByteArray &jpegData)
{
    QImage image;
    if (!image.loadFromData(jpegData, "JPEG")) {
        qWarning() << "Failed to decode JPEG image";
        return QImage();
    }
    return image;
}

void MjpegDecoder::setConnected(bool connected)
{
    if (m_connected != connected) {
        m_connected = connected;
        emit connectedChanged();
    }
}

void MjpegDecoder::updateFpsCounter()
{
    if (m_fps != m_frameCount) {
        m_fps = m_frameCount;
        emit fpsChanged();
    }
    m_frameCount = 0;
}
