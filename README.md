ffmpeg -f dshow -i video="UVC Camera" -vcodec libx264 -preset ultrafast -tune zerolatency -b:v 2M -maxrate 2M -bufsize 512k -pix_fmt yuv420p -g 30 -f rtp rtp://127.0.0.1:5004

connecting to the car using ssh:
ssh pi@<raspberry pi ip address> -p 22
password: raspberry

When switching on the car adjust BOTH!!!!! switches (rpi and servo which is located under the battery near yellow hazard sign on the power cable)

after firing up the car go to the mycar folder and run:
donkey calibrate --channel 0
check if this is responsible for steering by typing pwm values (at first stick to values between 230 and 430)
if this is rensponsible for throttle instead of steering got file mycar/config.py
and find steering channel value and change it from 1 to 0 and throttle channel value from 0 to 1

next go back to calibrating and find max left/right pwm values and adjust them in config.py file
for me it was (500 max left and 230 right)

if camera is flipped the wrong way go to config.py and adjust values:
CAMERA_VFLIP or CAMERA_HFLIP

Adjust also the throttle values (if the values for front drive and reverse are inverted switch places of the value in throttle front max and reverse max )

bacause of the old raspberry system these changes where needed:
    sudo nano /etc/apt/sources.list
    Replace the contents with these correct archived repository URLs:
    deb http://legacy.raspbian.org/raspbian/ buster main contrib non-free rpi
This change enables apt for old raspberry pi os
also these command was needed to use gstreamer: 
sudo apt install -y gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-omx gstreamer1.0-plugins-ugly

command on windows that worked
gst-launch-1.0 udpsrc port=5000 ! application/x-rtp,encoding-name=JPEG,payload=26 ! rtpjpegdepay ! jpegdec ! videoconvert ! autovideosink
command on linux that worked
gst-launch-1.0 v4l2src device=/dev/video0 ! video/x-raw,width=640,height=480,framerate=30/1 ! videoconvert ! jpegenc !  rtpjpegpay !  udpsink host=192.168.18.18 port=5000
