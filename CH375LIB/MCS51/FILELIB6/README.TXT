CH375为MCS51单片机提供了三个U盘文件级子程序库
  CH375HF3.LIB 支持FAT12/FAT16/FAT32文件系统,内部RAM存放变量,总线方式并口I/O,只读版
  CH375HF4.LIB 支持FAT12/FAT16/FAT32文件系统,内部RAM存放变量,总线方式并口I/O,针对CH375B优化版
  CH375HF6.LIB 支持FAT12/FAT16/FAT32文件系统,内部RAM存放变量,总线方式并口I/O
  CH375HF5.LIB 支持FAT12/FAT16/FAT32文件系统,内部RAM存放变量,串口或I/O模拟并口或高速并口
  CH375HFA.LIB 支持FAT12/FAT16/FAT32文件系统,外部RAM存放变量,总线方式并口I/O
  CH375HFC.LIB 支持FAT12/FAT16/FAT32文件系统,外部RAM存放变量,串口或I/O模拟并口或高速并口

其中:
  CH375HF3不支持写U盘,其它均与CH375HF6相同
  CH375HF4主要支持CH375B,部分支持CH375S/A/V,其它均与CH375HF6相同
  CH375HF3,CH375HF4,CH375HF6三者的对外接口和子程序调用方法完全相同,
  已经编译好的应用程序,只需要替换.H或.INC头文件以及子程序库文件重新链接即可.
