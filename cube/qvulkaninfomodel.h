#ifndef QVULKANINFOMODEL_H
#define QVULKANINFOMODEL_H
#include <QAbstractItemModel>

class TreeItem;
class QVkInstance;

class QVulkanInfoModel : public QAbstractItemModel
{
    Q_OBJECT

 public:
     explicit QVulkanInfoModel(QVkInstance* instance, QObject *parent = nullptr);
     ~QVulkanInfoModel();

     QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
     Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
     QVariant headerData(int section, Qt::Orientation orientation,
                         int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
     QModelIndex index(int row, int column,
                       const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
     QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
     int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
     int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;

 private:
     void setupModelData(QVkInstance* lines, TreeItem *parent);

     TreeItem *rootItem { nullptr };
};

#endif // QVULKANINFOMODEL_H
