#ifndef MJPEGDECODER_HPP
#define MJPEGDECODER_HPP

#include <QObject>
#include <QMutex>
#include <QImage>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QQuickImageProvider>
#include <QTimer>

class MjpegImageProvider : public QQuickImageProvider
{
public:
    MjpegImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    void updateImage(const QImage &image);

private:
    QImage m_image;
    mutable QMutex m_mutex;
};

class MjpegDecoder : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString url READ url WRITE setUrl NOTIFY urlChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(int fps READ fps NOTIFY fpsChanged)

public:
    explicit MjpegDecoder(QObject *parent = nullptr);
    ~MjpegDecoder();

    QString url() const { return m_url; }
    void setUrl(const QString &url);

    bool connected() const { return m_connected; }
    int fps() const { return m_fps; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void reconnect();

    MjpegImageProvider* imageProvider() { return m_imageProvider; }

signals:
    void urlChanged();
    void connectedChanged();
    void fpsChanged();
    void imageUpdated();
    void errorOccurred(const QString &error);

private slots:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError error);
    void updateFpsCounter();

private:
    void processBuffer();
    QImage decodeJpeg(const QByteArray &jpegData);
    void setConnected(bool connected);

    QString m_url;
    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_reply;
    QByteArray m_buffer;
    bool m_connected;

    // FPS tracking
    int m_fps;
    int m_frameCount;
    QTimer *m_fpsTimer;

    MjpegImageProvider *m_imageProvider;

    static const QByteArray BOUNDARY_START;
    static const QByteArray JPEG_START;
    static const QByteArray JPEG_END;
};

#endif // MJPEGDECODER_HPP
