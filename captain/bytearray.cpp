#include "include/bytearray.h"
#include <fstream>
#include <sstream>
#include <string.h>
#include <iomanip>

#include "include/endian.h"
#include "include/log.h"

namespace captain {

static captain::Logger::ptr g_logger = CAPTAIN_LOG_NAME("system");

ByteArray::Node::Node(size_t s)
    :ptr(new char[s])
    ,next(nullptr)
    ,size(s) {
}

//无参数的构造函数，用于创建一个空的节点。
ByteArray::Node::Node()
    :ptr(nullptr)
    ,next(nullptr)
    ,size(0) {
}

//析构函数 释放节点的资源
ByteArray::Node::~Node() {
    if(ptr) {
        delete[] ptr;
    }
}

ByteArray::ByteArray(size_t base_size)
    :m_baseSize(base_size)
    ,m_position(0)
    ,m_capacity(base_size)
    ,m_size(0)
    ,m_endian(CAPTAIN_BIG_ENDIAN)
    ,m_root(new Node(base_size))
    ,m_cur(m_root) {
}

//析构函数  销毁 ByteArray 对象及其节点
ByteArray::~ByteArray() {
    Node* tmp = m_root;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
}

//检查当前字节序是否为小端序
bool ByteArray::isLittleEndian() const {
    return m_endian == CAPTAIN_LITTLE_ENDIAN;
}

//设置字节序
void ByteArray::setIsLittleEndian(bool val) {
    if(val) {
        m_endian = CAPTAIN_LITTLE_ENDIAN;
    } else {
        m_endian = CAPTAIN_BIG_ENDIAN;
    }
}

//写入一个有符号 8 位整数 (int8_t) 到字节数组中。
void ByteArray::writeFint8  (int8_t value) {
    write(&value, sizeof(value));
}

//写入一个无符号 8 位整数 (uint8_t) 到字节数组中
void ByteArray::writeFuint8 (uint8_t value) {
    write(&value, sizeof(value));
}

//写入一个有符号 16 位整数 (int16_t) 到字节数组中。
void ByteArray::writeFint16 (int16_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//写入一个无符号 16 位整数 (uint16_t) 到字节数组中
void ByteArray::writeFuint16(uint16_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//写入一个有符号 32 位整数 (int32_t) 到字节数组中
void ByteArray::writeFint32 (int32_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//写入一个无符号 32 位整数 (uint32_t) 到字节数组中
void ByteArray::writeFuint32(uint32_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//写入有符号 64 位整数 (int64_t) 到字节数组中。
void ByteArray::writeFint64 (int64_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//写入无符号 64 位整数 (uint64_t) 到字节数组中。
void ByteArray::writeFuint64(uint64_t value) {
    if(m_endian != CAPTAIN_BYTE_ORDER) {
        value = byteswap(value);
    }
    write(&value, sizeof(value));
}

//对整数进行 Zigzag 编码和解码的函数。Zigzag 编码是一种方法，用于将有符号整数转换为无符号整数，以便更有效地在二进制格式中进行存储和传输。
static uint32_t EncodeZigzag32(const int32_t& v) {
    if(v < 0) {
        return ((uint32_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}

static uint64_t EncodeZigzag64(const int64_t& v) {
    if(v < 0) {
        return ((uint64_t)(-v)) * 2 - 1;
    } else {
        return v * 2;
    }
}
//Zigzag 编码并不会减小整数的实际值，它只是改变了表示方式，以在编码时节省空间。
//在解码时，可以还原回原始的有符号整数值。这种编码方法特别适用于需要高效地在网络传输或存储中表示整数的情况。
static int32_t DecodeZigzag32(const uint32_t& v) {
    return (v >> 1) ^ -(v & 1);
}

static int64_t DecodeZigzag64(const uint64_t& v) {
    return (v >> 1) ^ -(v & 1);
}


void ByteArray::writeInt32  (int32_t value) {
    writeUint32(EncodeZigzag32(value));
}

//通过循环迭代将一个无符号的 32 位整数进行 Base-128 Varint 编码，然后将编码后的数据写入 tmp 数组中。
void ByteArray::writeUint32 (uint32_t value) {
    uint8_t tmp[5];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

//将一个 64 位的带符号整数写入字节数组。首先对整数进行 Zigzag 编码，然后将编码后的无符号整数写入字节数组。
void ByteArray::writeInt64  (int64_t value) {
    writeUint64(EncodeZigzag64(value));
}

void ByteArray::writeUint64 (uint64_t value) {
    uint8_t tmp[10];
    uint8_t i = 0;
    while(value >= 0x80) {
        tmp[i++] = (value & 0x7F) | 0x80;
        value >>= 7;
    }
    tmp[i++] = value;
    write(tmp, i);
}

//将双精度浮点数写入字节数组
void ByteArray::writeFloat  (float value) {
    uint32_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint32(v);
}
//将双精度浮点数写入字节数组
void ByteArray::writeDouble (double value) {
    uint64_t v;
    memcpy(&v, &value, sizeof(value));
    writeFuint64(v);
}

//将字符串写入字节数组
void ByteArray::writeStringF16(const std::string& value) {
    writeFuint16(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF32(const std::string& value) {
    writeFuint32(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringF64(const std::string& value) {
    writeFuint64(value.size());
    write(value.c_str(), value.size());
}

void ByteArray::writeStringVint(const std::string& value) {
    writeUint64(value.size());
    write(value.c_str(), value.size());
}

//将给定的字符串写入 ByteArray，但不包括任何用于表示字符串长度的前缀。
void ByteArray::writeStringWithoutLength(const std::string& value) {
    write(value.c_str(), value.size());
}

//从 ByteArray 对象中读取一个 8 位有符号整数（int8_t）。
int8_t   ByteArray::readFint8() {
    int8_t v;
    read(&v, sizeof(v));
    return v;
}

//从 ByteArray 对象中读取一个 8 位无符号整数（uint8_t）。
uint8_t  ByteArray::readFuint8() {
    uint8_t v;
    read(&v, sizeof(v));
    return v;
}

#define XX(type) \
    type v; \
    read(&v, sizeof(v)); \
    if(m_endian == CAPTAIN_BYTE_ORDER) { \
        return v; \
    } else { \
        return byteswap(v); \
    }

int16_t  ByteArray::readFint16() {
    XX(int16_t);
}
uint16_t ByteArray::readFuint16() {
    XX(uint16_t);
}

int32_t  ByteArray::readFint32() {
    XX(int32_t);
}

uint32_t ByteArray::readFuint32() {
    XX(uint32_t);
}

int64_t  ByteArray::readFint64() {
    XX(int64_t);
}

uint64_t ByteArray::readFuint64() {
    XX(uint64_t);
}

#undef XX

//将 Zigzag 编码后的整数还原为原始的有符号整数值。
int32_t  ByteArray::readInt32() {
    return DecodeZigzag32(readUint32());
}
//实现了对 Base-128 Varint 编码的解码，可以有效地读取编码的无符号整数值。
uint32_t ByteArray::readUint32() {
    uint32_t result = 0;
    for(int i = 0; i < 32; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint32_t)b) << i;
            break;
        } else {
            result |= (((uint32_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

//读取一个 64 位有符号整数值
int64_t  ByteArray::readInt64() {
    return DecodeZigzag64(readUint64());
}

//从字节数组中读取一个 64 位无符号整数（uint64_t 类型）的值
uint64_t ByteArray::readUint64() {
    uint64_t result = 0;
    for(int i = 0; i < 64; i += 7) {
        uint8_t b = readFuint8();
        if(b < 0x80) {
            result |= ((uint64_t)b) << i;
            break;
        } else {
            result |= (((uint64_t)(b & 0x7f)) << i);
        }
    }
    return result;
}

//从字节数组中读取不同类型的数据
float    ByteArray::readFloat() {
    uint32_t v = readFuint32();
    float value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

double   ByteArray::readDouble() {
    uint64_t v = readFuint64();
    double value;
    memcpy(&value, &v, sizeof(v));
    return value;
}

std::string ByteArray::readStringF16() {
    uint16_t len = readFuint16();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF32() {
    uint32_t len = readFuint32();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringF64() {
    uint64_t len = readFuint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

std::string ByteArray::readStringVint() {
    uint64_t len = readUint64();
    std::string buff;
    buff.resize(len);
    read(&buff[0], len);
    return buff;
}

//将 ByteArray 对象重置为空
void ByteArray::clear() {
    m_position = m_size = 0;
    m_capacity = m_baseSize;
    Node* tmp = m_root->next;
    while(tmp) {
        m_cur = tmp;
        tmp = tmp->next;
        delete m_cur;
    }
    m_cur = m_root;
    m_root->next = NULL;
}

//将数据从给定的缓冲区 buf 写入到 ByteArray 对象中
void ByteArray::write(const void* buf, size_t size) {
    if(size == 0) {
        return;
    }
    // 确保 ByteArray 的容量足够大以容纳要写入的数据
    addCapacity(size);
    // 计算当前操作位置相对于当前节点的起始位置的偏移量
    size_t npos = m_position % m_baseSize;
    // 计算当前节点剩余可写入的字节数
    size_t ncap = m_cur->size - npos;
    // 初始化一个变量，用于跟踪要从源缓冲区 buf 中复制的字节数
    size_t bpos = 0;
    // 进入循环，循环条件是 size > 0，即还有数据需要写入
    while(size > 0) {
        if(ncap >= size) {
            // 当前节点剩余容量足够大以容纳要写入的数据
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, size);
            // 如果当前节点被填满，切换到下一个节点
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            // 更新位置、偏移和剩余数据大小
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            // 当前节点的剩余容量不足以容纳要写入的数据
            memcpy(m_cur->ptr + npos, (const char*)buf + bpos, ncap);
            // 更新位置、偏移和剩余数据大小
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            // 切换到下一个节点
            m_cur = m_cur->next;
            // 获取下一个节点的剩余容量和起始位置
            ncap = m_cur->size;
            npos = 0;
        }
    }
    //如果写入操作使得当前位置 m_position 超过了当前数据的大小 m_size，
    // 则更新 m_size 为当前位置，以反映数据的实际大小
    if(m_position > m_size) {
        m_size = m_position;
    }
}

//从 ByteArray 对象中读取指定大小的数据到目标缓冲区。
void ByteArray::read(void* buf, size_t size) {
    // 检查要读取的数据大小是否超过了可读取数据的范围
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = m_position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    // 进入循环，循环条件是 size > 0，即还有数据需要写入
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, size);
            if(m_cur->size == (npos + size)) {
                m_cur = m_cur->next;
            }
            m_position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, m_cur->ptr + npos, ncap);
            m_position += ncap;
            bpos += ncap;
            size -= ncap;
            m_cur = m_cur->next;
            ncap = m_cur->size;
            npos = 0;
        }
    }
}

//从指定位置读取指定大小的数据到缓冲区中。不改变数据。
void ByteArray::read(void* buf, size_t size, size_t position) const {
    if(size > getReadSize()) {
        throw std::out_of_range("not enough len");
    }

    size_t npos = position % m_baseSize;
    size_t ncap = m_cur->size - npos;
    size_t bpos = 0;
    Node* cur = m_cur;
    while(size > 0) {
        if(ncap >= size) {
            memcpy((char*)buf + bpos, cur->ptr + npos, size);
            if(cur->size == (npos + size)) {
                cur = cur->next;
            }
            position += size;
            bpos += size;
            size = 0;
        } else {
            memcpy((char*)buf + bpos, cur->ptr + npos, ncap);
            position += ncap;
            bpos += ncap;
            size -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
    }
}

//设置 ByteArray 对象的当前操作位置。
void ByteArray::setPosition(size_t v) {
    /// 检查新的操作位置是否超出了可操作范围
    if(v > m_capacity) {
        throw std::out_of_range("set_position out of range");
    }
    // 设置新的操作位置
    m_position = v;
    // 如果新的操作位置大于当前的大小，更新大小
    if(m_position > m_size) {
        m_size = m_position;
    }
    // 重置当前节点为根节点
    m_cur = m_root;
    // 遍历节点链表，寻找包含新的操作位置的节点
    while(v > m_cur->size) {
        v -= m_cur->size;
        m_cur = m_cur->next;
    }
     // 如果新的操作位置正好等于当前节点的大小，切换到下一个节点
    if(v == m_cur->size) {
        m_cur = m_cur->next;
    }
}

//将 ByteArray 对象的数据写入到文件中。
bool ByteArray::writeToFile(const std::string& name) const {
    // 打开文件，使用二进制写入模式和截断模式
    std::ofstream ofs;
    ofs.open(name, std::ios::trunc | std::ios::binary);
    // 检查文件是否成功打开
    if(!ofs) {
        CAPTAIN_LOG_ERROR(g_logger) << "writeToFile name=" << name
            << " error , errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }
    // 获取可读取的数据大小和当前操作位置
    int64_t read_size = getReadSize();
    int64_t pos = m_position;
    Node* cur = m_cur;

    // 循环将数据写入文件
    while(read_size > 0) {
        //取出偏移量
        int diff = pos % m_baseSize;
        //计算长度  读的长度比每个链表的长度大，就取链表长度，否则取要读的长度，再减去偏移量
        int64_t len = (read_size > (int64_t)m_baseSize ? m_baseSize : read_size) - diff;
        ofs.write(cur->ptr + diff, len);
        cur = cur->next;
        pos += len;
        read_size -= len;
    }

    return true;
}

//从文件中读取数据并填充到 ByteArray 对象中。
bool ByteArray::readFromFile(const std::string& name) {
    // 打开文件，使用二进制读取模式
    std::ifstream ifs;
    ifs.open(name, std::ios::binary);
    // 检查文件是否成功打开
    if(!ifs) {
        CAPTAIN_LOG_ERROR(g_logger) << "readFromFile name=" << name
            << " error, errno=" << errno << " errstr=" << strerror(errno);
        return false;
    }

    // 使用 shared_ptr 分配缓冲区，确保在函数退出时自动释放内存
    std::shared_ptr<char> buff(new char[m_baseSize], [](char* ptr) { delete[] ptr;});
    // 循环读取文件内容并写入 ByteArray 对象
    while(!ifs.eof()) {
        ifs.read(buff.get(), m_baseSize);
        write(buff.get(), ifs.gcount());
    }
    return true;
}

//增加 ByteArray 对象的容量以容纳指定大小的数据。
void ByteArray::addCapacity(size_t size) {
    // 如果要添加的容量大小为0，则直接返回，无需增加容量
    if(size == 0) {
        return;
    }
    // 获取当前的剩余容量
    size_t old_cap = getCapacity();
    // 如果当前剩余容量已经足够大，无需增加容量
    if(old_cap >= size) {
        return;
    }

    // 计算需要增加的节点数量，确保容量足够
    size = size - old_cap;
    size_t count = (size / m_baseSize) + (((size % m_baseSize) > old_cap) ? 1 : 0);

    // 遍历到链表的末尾
    Node* tmp = m_root;
    while(tmp->next) {
        tmp = tmp->next;
    }

    Node* first = NULL;
    // 为每个需要增加的节点分配内存并连接到链表中
    for(size_t i = 0; i < count; ++i) {
        tmp->next = new Node(m_baseSize);
        if(first == NULL) {
            first = tmp->next;
        }
        tmp = tmp->next;
        // 增加总容量
        m_capacity += m_baseSize;
    }

    // 如果原始容量为0，将当前节点指向第一个新增节点
    if(old_cap == 0) {
        m_cur = first;
    }
}

//从当前位置读取字节数组的内容并返回一个字符串表示。
std::string ByteArray::toString() const {
    // 创建一个字符串，大小为当前可读取的字节数
    std::string str;
    str.resize(getReadSize());

    // 如果字符串为空，直接返回空字符串
    if(str.empty()) {
        return str;
    }

    // 从当前位置读取数据到字符串中
    read(&str[0], str.size(), m_position);
    return str;
}

//将字节数组中的数据转换为十六进制字符串表示形式。
std::string ByteArray::toHexString() const {
    // 获取字节数组的字符串表示形式
    std::string str = toString();
    std::stringstream ss;

    for(size_t i = 0; i < str.size(); ++i) {
        if(i > 0 && i % 32 == 0) {
            ss << std::endl; // 每32个字符换行
        }
        // 格式化为两位十六进制数并添加到流中
        ss << std::setw(2) << std::setfill('0') << std::hex
           << (int)(uint8_t)str[i] << " ";
    }

    return ss.str();
}


uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers, uint64_t len) const {
    // 确保请求的长度不超过可读数据的长度
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;
    // 计算当前位置在当前节点中的偏移
    size_t npos = m_position % m_baseSize;
    // 计算当前节点中剩余的可读数据长度
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;

    while(len > 0) {
        if(ncap >= len) {
            // 当前节点剩余的数据足够满足请求的长度
            iov.iov_base = cur->ptr + npos; // 设置iov的起始地址
            iov.iov_len = len; // 设置iov的长度为请求的长度
            len = 0;  // 请求的数据已经全部满足，结束循
        } else {
            // 当前节点的剩余数据不足以满足请求的长度
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap; // 减去已满足的长度
            cur = cur->next; // 切换到下一个节点
            ncap = cur->size; // 更新节点的可用数据长度
            npos = 0; // 重置偏移为0，因为切换了节点
        }
        buffers.push_back(iov); // 将iov添加到输出的buffers中
    }
    return size; // 返回实际添加到buffers中的数据长度
}

// ByteArray::getReadBuffers 函数的重载版本，允许指定起始位置 position 来获取可读数据的一组 iovec 片段。
uint64_t ByteArray::getReadBuffers(std::vector<iovec>& buffers
                                ,uint64_t len, uint64_t position) const {
    len = len > getReadSize() ? getReadSize() : len;
    if(len == 0) {
        return 0;
    }

    uint64_t size = len;
    // 计算指定位置在当前节点中的偏移
    size_t npos = position % m_baseSize;
     // 计算指定位置在当前节点链表中的节点数
    size_t count = position / m_baseSize;
    Node* cur = m_root;
    while(count > 0) {
        cur = cur->next;
        --count;
    }

    size_t ncap = cur->size - npos;
    struct iovec iov;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;
            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

uint64_t ByteArray::getWriteBuffers(std::vector<iovec>& buffers, uint64_t len) {
    if(len == 0) {
        return 0;
    }

    // 添加足够的容量以容纳请求的长度
    addCapacity(len);
    uint64_t size = len;

    // 计算当前位置在当前节点中的偏移
    size_t npos = m_position % m_baseSize;
    // 计算当前位置到当前节点末尾的剩余容量
    size_t ncap = m_cur->size - npos;
    struct iovec iov;
    Node* cur = m_cur;
    while(len > 0) {
        if(ncap >= len) {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = len;
            len = 0;
        } else {
            iov.iov_base = cur->ptr + npos;
            iov.iov_len = ncap;

            len -= ncap;
            cur = cur->next;
            ncap = cur->size;
            npos = 0;
        }
        buffers.push_back(iov);
    }
    return size;
}

}
