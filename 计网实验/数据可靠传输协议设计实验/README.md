## 开发平台

Visual Studio 2022

## 编译步骤

用Visual Studio 2022打开`Rdt.sln`文件，将上方解决方案配置改为Debug，解决方案平台改为x86，在“生成”中选择Build Rdt即可。

似乎由于`netsimlib.lib`的一些问题，不能以Release方式编译。

## 运行说明

可执行码中包含了三个exe（均为Debug, x86版本）和对应的bat，可以通过运行对应的bat文件来运行这些exe。`input.txt`，`output.txt`是共用的。