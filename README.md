# serverFramework
high-performance server development——learning journey

### Day1

添加了日志的基本框架，实现了LogAppender，及其派生类StdoutLogAppender、FileLogAppender

> little tips：return !!m_filestream; //双感叹号，非0转成1，0值还是0

### Day2

日志框架初步完成，并进行了编译调试。将代码中的宏定义写法改写为常规写法，抛弃了花哨的技巧，让代码通俗易懂。

### Day3

完善日志系统。

### Day4

基于yaml-cpp，初步配置了yaml，可以从yml文件中读取基本类型数据。\

### Day5

完善配置系统，支持更复杂的类型，如stl类型、自定义类型。

添加了配置系统的事件机制，当一个配置项发生更改的时候，反向通知对应的代码