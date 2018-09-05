APP_PLATFORM := android-24
APP_ABI := arm64-v8a armeabi-v7a

APP_CPPFLAGS += -std=c++11
APP_STL := c++_static

APP_CFLAGS += \
	-Ofast \
	-funroll-loops \
	-fno-strict-aliasing \
	-mfpu=neon-vfpv4 \
	-mfloat-abi=softfp \
	-DHAVE_NEON=1 \
