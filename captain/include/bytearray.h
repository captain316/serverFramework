#ifndef __CAPTAIN_BYTEARRAY_H__
#define __CAPTAIN_BYTEARRAY_H__

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>

namespace captain {

//提供了一组函数来读取和写入不同类型的数据，并且可以自动扩展内存以容纳更多的数据。这个类可以用于处理二进制数据的序列化和反序列化等任务。
class ByteArray {
public:
    //使用智能指针来管理 ByteArray 对象的生命周期。
    typedef std::shared_ptr<ByteArray> ptr;
    
    //自动扩展内存的核心思想是基于链表结构来管理数据块（节点）
    //Node 结构表示 ByteArray 中的一个节点，每个节点都包含一个指向字符数组的指针（ptr）、一个指向下一个节点的指针（next）以及该节点中字符数组的大小（size）。
    struct Node {
        Node(size_t s);
        Node();
        ~Node();

        char* ptr;
        Node* next;
        size_t size;
    };

    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //write
    void writeFint8  (int8_t value);
    void writeFuint8 (uint8_t value);
    void writeFint16 (int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32 (int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64 (int64_t value);
    void writeFuint64(uint64_t value);

    void writeInt32  (int32_t value);
    void writeUint32 (uint32_t value);
    void writeInt64  (int64_t value);
    void writeUint64 (uint64_t value);

    void writeFloat  (float value);
    void writeDouble (double value);
    //length:int16 , data
    void writeStringF16(const std::string& value);
    //length:int32 , data
    void writeStringF32(const std::string& value);
    //length:int64 , data
    void writeStringF64(const std::string& value);
    //length:varint, data
    void writeStringVint(const std::string& value);
    //data
    void writeStringWithoutLength(const std::string& value);

    //read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();

    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();

    float    readFloat();
    double   readDouble();

    //length:int16, data
    std::string readStringF16();
    //length:int32, data
    std::string readStringF32();
    //length:int64, data
    std::string readStringF64();
    //length:varint , data
    std::string readStringVint();

    //内部操作
    void clear();

    void write(const void* buf, size_t size);
    void read(void* buf, size_t size);
    void read(void* buf, size_t size, size_t position) const;

    size_t getPosition() const { return m_position;}
    void setPosition(size_t v);

    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);

    size_t getBaseSize() const { return m_baseSize;}
    //还可以读多少数据
    size_t getReadSize() const { return m_size - m_position;}

    bool isLittleEndian() const;
    void setIsLittleEndian(bool val);

    std::string toString() const;
    std::string toHexString() const;

    //只获取内容，不修改position
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    //增加容量，不修改position
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);

    size_t getSize() const { return m_size;}
private:
    //添加容量
    void addCapacity(size_t size);
    //返回【剩余容量】
    size_t getCapacity() const { return m_capacity - m_position;}
private:
    size_t m_baseSize; //每一个node的大小
    size_t m_position; //当前操作位置
    size_t m_capacity; //容量
    size_t m_size;     //当前的真实大小
    int8_t m_endian;   
    Node* m_root;
    Node* m_cur;       //当前节点
};

}

#endif
