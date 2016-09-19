#include "qvulkaninfomodel.h"
#include <QStringList>
#include <QList>
#include <QVariant>
#include "qvkinstance.h"

class TreeItem
{
public:
    explicit TreeItem(const QList<QVariant> &data, TreeItem *parentItem = nullptr) {
        m_parentItem = parentItem;
        m_itemData = data;
    }

    ~TreeItem() {
        qDeleteAll(m_childItems);
    }

    void appendChild(TreeItem *child) {
        child->m_parentItem = this;
        m_childItems.append(child);
    }

    TreeItem *child(int row) {
        return m_childItems.value(row);
    }

    int childCount() const {
        return m_childItems.count();
    }

    int columnCount() const {
        return m_itemData.count();
    }
    QVariant data(int column) const {
        return m_itemData.value(column);
    }

    int row() const {
        if (m_parentItem)
            return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));

        return 0;
    }

    TreeItem *parentItem() {
        return m_parentItem;
    }

private:
    QList<TreeItem*> m_childItems;
    QList<QVariant> m_itemData;
    TreeItem *m_parentItem;
};

class DeviceNode: public TreeItem {
public:
    class LimitsNode: public TreeItem {
    public:
        LimitsNode(VkPhysicalDeviceLimits limits, TreeItem* parent = nullptr)
            : TreeItem({"limits", "value"}, parent) {
            appendChild(new TreeItem({"bufferImageGranularity", (uint)limits.bufferImageGranularity}));
        }
    };

    DeviceNode(QVkPhysicalDevice* dev, TreeItem* parent = nullptr)
        : TreeItem({dev->properties().deviceName, "value"}, parent) {
        VkPhysicalDeviceProperties p = dev->properties();
        appendChild(new TreeItem({"deviceType",     p.deviceType}));
        appendChild(new TreeItem({"deviceID",       p.deviceID}));
        appendChild(new TreeItem({"apiVersion",     p.apiVersion}));
        appendChild(new TreeItem({"vendorID",       p.vendorID}));
        appendChild(new LimitsNode(p.limits));
    }
};


QVulkanInfoModel::QVulkanInfoModel(QVkInstance* instance, QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new TreeItem({"Element", "Summary"});
    setupModelData(instance, rootItem);
}

QVulkanInfoModel::~QVulkanInfoModel()
{
    delete rootItem;
}

int QVulkanInfoModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return static_cast<TreeItem*>(parent.internalPointer())->columnCount();
    else
        return rootItem->columnCount();
}

QVariant QVulkanInfoModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole)
        return QVariant();

    TreeItem *item = static_cast<TreeItem*>(index.internalPointer());

    return item->data(index.column());
}

Qt::ItemFlags QVulkanInfoModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return nullptr;

    return QAbstractItemModel::flags(index);
}

QVariant QVulkanInfoModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return rootItem->data(section);

    return QVariant();
}

QModelIndex QVulkanInfoModel::index(int row, int column, const QModelIndex &parent)
            const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    TreeItem *parentItem;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    TreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

QModelIndex QVulkanInfoModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    TreeItem *childItem = static_cast<TreeItem*>(index.internalPointer());
    TreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(parentItem->row(), 0, parentItem);
}

int QVulkanInfoModel::rowCount(const QModelIndex &parent) const
{
    TreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = rootItem;
    else
        parentItem = static_cast<TreeItem*>(parent.internalPointer());

    return parentItem->childCount();
}

void QVulkanInfoModel::setupModelData(QVkInstance *instance, TreeItem *root)
{
    TreeItem* parent;

    parent = new TreeItem({"enabled layers", ""});
    root->appendChild(parent);

    for(auto name: instance->enabledLayerNames()) {
        TreeItem* i = new TreeItem({name, ""});
        parent->appendChild(i);
    }

    parent = new TreeItem({"enabled extensions",""});
    root->appendChild(parent);
    for(auto name: instance->enabledExtensionNames()) {
        TreeItem* i = new TreeItem({name,""});
        parent->appendChild(i);
    }

    parent = new TreeItem({"available layers", ""});
    root->appendChild(parent);
    for(auto elem: instance->availableLayers()) {
        TreeItem* i = new TreeItem({elem.layerName, ""});
        parent->appendChild(i);
    }

    parent = new TreeItem({"available extensions",""});
    root->appendChild(parent);
    for(auto elem: instance->availableExtensions()) {
        TreeItem* i = new TreeItem({elem.extensionName,""});
        parent->appendChild(i);
    }

    TreeItem* devices = new TreeItem({"devices","available physical devices"});
    root->appendChild(devices);
    int numDevices = instance->numDevices();
    for(int j = 0; j < numDevices; j++) {
        QVkPhysicalDevice dev = instance->device(j);
        TreeItem* i = new DeviceNode(&dev);
        devices->appendChild(i);
    }
}
