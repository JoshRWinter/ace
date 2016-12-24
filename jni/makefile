activity=ace

all:../assets/assets
	NDK_DEBUG=1 ndk-build -C ..
	ant debug -f ../build.xml
	adb install -r $(CURDIR)/../bin/$(activity)-debug.apk
	adb shell am start -n joshwinter.$(activity)/android.app.NativeActivity
	
../assets/assets:../assets_local/build.txt ../assets_local/*
	cd ../assets_local && ./convert.sh
	cd ../assets_local/ && pack --build

#../assets/uiassets:../assets_local/uibuild.txt ../assets_local/*.tga
#	cd ../assets_local/ && pack --build uibuild.txt

#../assets/aassets:../assets_local/apack.txt ../assets_local/*.ogg
#	cd ../assets_local/ && apack --build
