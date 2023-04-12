# compiling

1) Clone a fresh copy of Tasmota, optionally checking out a known tag:
```
$ git clone https://github.com/arendst/Tasmota.git
$ git fetch --tags
$ git checkout v12.4.0
```

2) Copy the files provided in this repository into the Tasmota source repository:
```
$ cp ./tasmota-eachen-curtain/platformio_override.ini ./Tasmota/
$ cp ./tasmota-eachen-curtain/platformio_tasmota_cenv.ini ./Tasmota/
$ cp -r ./tasmota-eachen-curtain/tasmota ./Tasmota
```

3) Assuming you already have the PlatformIO extension installed in Visual Studio Code, open the cloned Tasmota repo:
```
$ ./code .
```

4) You should now be able to start a build:
```
Goto: View -> Command Pallette
Type in: "Build"
Select: "PlatformIO: Build"
```
5) The resulting tasmota.bin file should be found under
```
./Tasmota/build_output/firmware/tasmota.bin
```