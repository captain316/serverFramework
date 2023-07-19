# serverFramework
high-performance server development——learning journey

### Day1

添加了日志的基本框架，实现了LogAppender，及其派生类StdoutLogAppender、FileLogAppender

> little tips：return !!m_filestream; //双感叹号，非0转成1，0值还是0

### Day2

日志框架初步完成，并进行了编译调试。将代码中的宏定义写法改写为常规写法，抛弃了花哨的技巧，让代码通俗易懂。