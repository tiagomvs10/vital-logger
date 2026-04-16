
BUILDROOT_DIR=/home/tiago/buildroot/buildroot-2025.02.6

PI_IP="myRaspberry.local"       
PI_USER="root"           
OUTPUT_BIN="VitalLogger" # executable file name

# source files separated by spaces; header files in the same path
SOURCE_FILES="sensor.cpp signalProcessor.cpp t_ReadSensor.cpp t_Process.cpp vitalLogger.cpp main.cpp raspAppConnection.cpp t_CmdParser.cpp t_RTDisplay.cpp t_DBRegister.cpp t_FaceAuthenticator.cpp camera.cpp buzzer.cpp faceVerifier.cpp raspCloudConnection.cpp ppgStrategy.cpp"

COMPILER="$BUILDROOT_DIR/output/host/bin/aarch64-buildroot-linux-gnu-g++"
#COMPILER=g++
SYSROOT="$BUILDROOT_DIR/output/staging"

#FLAGS="-std=c++11 -pthread"
FLAGS="-std=c++11 -pthread -O3 -mcpu=cortex-a72 -mtune=cortex-a72 -ftree-vectorize -funsafe-math-optimizations"

INCLUDES="-I$SYSROOT/usr/include/opencv4"
LIB_PATH="-L$SYSROOT/usr/lib"

# OpenCV libraries
LIBS="-lopencv_core -lopencv_imgproc -lopencv_imgcodecs -lopencv_videoio -lopencv_dnn -lopencv_objdetect -lcurl -lrt -lpthread"

# compile command
$COMPILER $SOURCE_FILES -o $OUTPUT_BIN \
    --sysroot="$SYSROOT" \
    $FLAGS \
    $INCLUDES \
    $LIB_PATH \
    $LIBS

if [ $? -ne 0 ]; then
    echo "compilation error"
    exit 1
fi

echo "compilation sucess: $OUTPUT_BIN"

# deploy in Raspberry (will ask for password)

scp $OUTPUT_BIN $PI_USER@$PI_IP:/root/

if [ $? -ne 0 ]; then
    echo "deploy error to $PI_IP."
    exit 1
fi

# Dar permissão de execução remotamente
ssh $PI_USER@$PI_IP "chmod +x /root/$OUTPUT_BIN"

echo "sucess. to execute, connect to Raspberry pi and execute /root/$OUTPUT_BIN"
