#ifndef METADATADIALOG_H
#define METADATADIALOG_H

#include <QDialog>
#include <QMediaCaptureSession>
#include <QMediaMetaData>
#include <QLineEdit>

class MetaDataDialog : public QDialog
{
public:
    MetaDataDialog();
    explicit MetaDataDialog(QWidget *parent = nullptr);
    QLineEdit *m_metaDataFields[QMediaMetaData::NumMetaData] = {};

private slots:
    void openThumbnailImage();
    void openCoverArtImage();
};

#endif // METADATADIALOG_H
