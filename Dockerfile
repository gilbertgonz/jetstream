FROM ubuntu:jammy

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential cmake libopencv-dev gdb x11-apps \
    # installing gstreamer
    gstreamer1.0-tools gstreamer1.0-plugins-base gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
    gstreamer1.0-plugins-nvvideo4linux2

COPY main.cpp /main.cpp
COPY CMakeLists.txt /CMakeLists.txt

RUN mkdir build && cd build \
    && cmake .. && make

RUN mkdir /vid

# Clean up
RUN apt remove -y build-essential cmake

CMD [ "./build/main" ]