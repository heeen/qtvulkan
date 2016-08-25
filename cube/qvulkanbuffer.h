#ifndef QVULKANBUFFER_H
#define QVULKANBUFFER_H
#include <vulkan/vulkan.h>
#include <qvkutil.h>
class QVkDeviceMemory: public QVkDeviceResource
{
public:
    QVkDeviceMemory(QVkDevice device)
        : QVkDeviceResource(device)
    {
        DEBUG_ENTRY;
    }

    void alloc(VkDeviceSize size, uint32_t typeIndex) {
    DEBUG_ENTRY;
        m_size = size;
        VkMemoryAllocateInfo allocinfo = {};
        allocinfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocinfo.allocationSize = m_size;
        allocinfo.memoryTypeIndex = typeIndex;

        VkResult err = vkAllocateMemory(m_device, &allocinfo, nullptr, &m_memory);
        Q_ASSERT(!err);
    }

    ~QVkDeviceMemory() {
    DEBUG_ENTRY;
        if(m_size)
            vkFreeMemory(m_device, m_memory, nullptr);
        m_size = 0;
    }

    void* map() { return map(0, m_size); }

    void* map(VkDeviceSize offset, VkDeviceSize size) {
        Q_ASSERT(offset + size <= m_size);
        void* ptr;
        VkMemoryMapFlags flags {0};
        vkMapMemory(m_device, m_memory, offset, size, flags, &ptr);
        return ptr;
    }

    void unmap() {
    DEBUG_ENTRY;
        vkUnmapMemory(m_device, m_memory);
    }

    VkDeviceSize size() {
        return m_size;
    }

protected:
    VkDeviceSize m_size { 0 };
    VkDeviceMemory m_memory { nullptr };
};


class QVkBuffer
: public QVkDeviceResource {
    public:
    QVkBuffer(QVkDevice device, VkDeviceSize size, VkBufferUsageFlags usage)
        : QVkDeviceResource(device) {
    DEBUG_ENTRY;

        VkResult err;
        VkBufferCreateInfo buf_ci = {};
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.size = size;
        buf_ci.usage = usage;
        // Create a host-visible buffer to copy the vertex data to (staging buffer)
        err = vkCreateBuffer(device, &buf_ci, nullptr, &m_buffer);
        Q_ASSERT(!err);
        vkGetBufferMemoryRequirements(device, m_buffer, &m_memReqs);
    }

    ~QVkBuffer() {
    DEBUG_ENTRY;
        vkDestroyBuffer(device(), m_buffer, nullptr);
    }

    VkDeviceSize size() {
        return m_memReqs.size;
    }

    VkBuffer buffer() {
        return m_buffer;
    }

protected:
    VkBuffer m_buffer;
    VkMemoryRequirements m_memReqs;
};

class QVkStagingBuffer: public QVkBuffer {
public:
    QVkStagingBuffer(QVkDevice device, size_t size)
        : QVkBuffer(device,size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT) {
    DEBUG_ENTRY;

        VkResult err;
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = m_memReqs.size;
        // Request a host visible memory type that can be used to copy our data do
        // Also request it to be coherent, so that writes are visible to the GPU right after unmapping the buffer
//        memAlloc.memoryTypeIndex = getMemoryTypeIndex(m_memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        err = vkAllocateMemory(device, &memAlloc, nullptr, &m_memory);
        Q_ASSERT(!err);
        // Map and copy
        void* data;
        err = vkMapMemory(device, m_memory, 0, memAlloc.allocationSize, 0, &data);
        Q_ASSERT(!err);
//        memcpy(data, vertexBuffer.data(), vertexBufferSize);
        vkUnmapMemory(device, m_memory);
        err = vkBindBufferMemory(device, m_buffer, m_memory, 0);
        Q_ASSERT(!err);
    }
    ~QVkStagingBuffer() {
    DEBUG_ENTRY;

    }

private:
    VkDeviceMemory m_memory;
};

class QVkDeviceBuffer
    : public QVkBuffer {
public:
    QVkDeviceBuffer(QVkDevice device, VkDeviceSize size, VkBufferUsageFlags usage)
        : QVkBuffer(device, size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage) {
    DEBUG_ENTRY;
    }

    ~QVkDeviceBuffer() {
    DEBUG_ENTRY;

    }
};

template <typename VT> class QVkVertexBuffer
    : public QVkDeviceBuffer {
public:
    QVkVertexBuffer(VkDevice device, uint32_t verts)
        : QVkDeviceBuffer(device, sizeof(VT) * verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT) {
    DEBUG_ENTRY;
    }

    ~QVkVertexBuffer() {
    DEBUG_ENTRY;

    }
};


template <typename UniformStruct>
class QVkUniformBuffer :public QVkDeviceResource {
public:
    QVkUniformBuffer(QVkDevice device)
        : QVkDeviceResource(device)
    {
        DEBUG_ENTRY;
        VkResult err;
        VkBufferCreateInfo buf_ci = {};
        buf_ci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buf_ci.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        buf_ci.size = sizeof(UniformStruct);
        err = vkCreateBuffer(m_device, &buf_ci, nullptr, &m_buffer);
        Q_ASSERT(!err);

        VkMemoryRequirements mem_reqs = {};
        vkGetBufferMemoryRequirements(m_device, m_buffer, &mem_reqs);

        VkMemoryAllocateInfo mem_ai;
        mem_ai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        mem_ai.pNext = nullptr;
        mem_ai.allocationSize = mem_reqs.size;
        mem_ai.memoryTypeIndex = 0;

        int index = device.memoryType(
           mem_reqs.memoryTypeBits,
           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
        Q_ASSERT(index > 0);
        mem_ai.memoryTypeIndex = index;

        err = vkAllocateMemory(m_device, &mem_ai, nullptr, &m_memory);
        m_allocationSize = mem_ai.allocationSize;
        Q_ASSERT(!err);


        err = vkBindBufferMemory(m_device, m_buffer, m_memory, /*offset*/ 0);
        Q_ASSERT(!err);

        m_descriptorInfo.buffer = m_buffer;
        m_descriptorInfo.offset = 0;
        m_descriptorInfo.range = sizeof(UniformStruct);

    }
    ~QVkUniformBuffer() {
        DEBUG_ENTRY;
        vkDestroyBuffer(m_device, m_buffer, nullptr);
        vkFreeMemory(m_device, m_memory, nullptr);
    }

    UniformStruct* map() {
        DEBUG_ENTRY;
        VkResult err;
        UniformStruct* mappedAddr;
        err = vkMapMemory(m_device, m_memory,
                          /*offset*/ 0,
                          m_allocationSize,
                          /*flags*/0,
                          (void **)&mappedAddr);
        Q_ASSERT(!err);
        return mappedAddr;
    }

    void unmap() {
        DEBUG_ENTRY;
        vkUnmapMemory(device(), m_memory);
    }

    void update() {
        DEBUG_ENTRY;
        UniformStruct* deviceData = map();
        memcpy(deviceData, m_hostData, sizeof(UniformStruct) );
        unmap();
    }

    UniformStruct* data() {
        return &m_hostData;
    }

    VkDescriptorBufferInfo* descriptorInfo() {
        DEBUG_ENTRY;
        return &m_descriptorInfo;
    }

protected:
    VkBuffer m_buffer { };
    VkDeviceMemory m_memory { };

    VkDeviceSize m_allocationSize { 0 };

    UniformStruct m_hostData { };

    VkDescriptorBufferInfo m_descriptorInfo { };

/*    class Accessor { //FIXME something about move semantics
    public:
        Accessor(QVkUniformBuffer& ubuf)
            : m_ubuf(ubuf) {
            m_mappedAddr = m_ubuf.map();
        }

        ~Accessor() {
            m_ubuf.unmap();
        }

        operator UniformStruct*() {
            return m_mappedAddr;
        }

    private:
        Accessor(Accessor&) = delete;
        Accessor& operator=(Accessor&) = delete; // fixme make copyable
        QVkUniformBuffer& m_ubuf;
        UniformStruct* m_mappedAddr;
    };*/
};


#endif // QVULKANBUFFER_H
