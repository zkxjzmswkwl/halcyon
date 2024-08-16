# Halcyon

If software *can* be fast, it *should* be fast. If a piece of software *can* be fast, but isn't, it is a bad piece of software.

One should not be forced to choose between losing contact with friends or using horrible software. Participating in modern social circles should not be a compromise.

Also I need this to keep in touch with work while travelling on an old Thinkpad.

# Build

Currently requires vcpkg (Windows). Will address that once the project is ready for use.

The following are currently used, but will be removed soon enough:

```
git clone https://github.com/zkxjzmswkwl/halcyon
cd halcyon
mkdir build
vcpkg install cpr:x64-windows
vcpkg install rapidjson:x64-windows
cmake ..
cmake --build . --config=RelWithDebInfo
```

# Image

![image](https://i.imgur.com/MofeafM.png)
