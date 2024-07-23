#include "imagesettings.h"
#include "ui_imagesettings.h"

#include <QCamera>

ImageSettings::ImageSettings(QImageCapture *imageCapture,QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageSettings),
    imagecapture(imageCapture)
{
    ui->setupUi(this);

    //image codecs
    ui->imageCodecBox->addItem(tr("Default image format"), QVariant(QString()));
    const auto supportedImageFormats = QImageCapture::supportedFormats();

    //遍历设备支持的图像格式，并将支持的图像格式使用ComBox下来菜单显示
    for (const auto &f : supportedImageFormats) {
        QString description = QImageCapture::fileFormatDescription(f);
        ui->imageCodecBox->addItem(QImageCapture::fileFormatName(f) + ": " + description, QVariant::fromValue(f));
    }

    //设置滑动条的范围
    ui->imageQualitySlider->setRange(0, int(QImageCapture::VeryHighQuality));

    //根据设备的支持的分辨率对其ComBox设置其下拉菜单
    ui->imageResolutionBox->addItem(tr("Default Resolution"));
    const QList<QSize> supportedResolutions = imagecapture->captureSession()->camera()->cameraDevice().photoResolutions();
    for (const QSize &resolution : supportedResolutions) {
        ui->imageResolutionBox->addItem(QString("%1x%2").arg(resolution.width()).arg(resolution.height()),
                                        QVariant(resolution));
    }

    selectComboBoxItem(ui->imageCodecBox, QVariant::fromValue(imagecapture->fileFormat()));
    selectComboBoxItem(ui->imageResolutionBox, QVariant(imagecapture->resolution()));
    ui->imageQualitySlider->setValue(imagecapture->quality());

    connect(ui -> buttonBox, &QDialogButtonBox::accepted, this, &ImageSettings::accept);
    connect(ui -> buttonBox, &QDialogButtonBox::rejected, this, &ImageSettings::reject);
}

ImageSettings::~ImageSettings()
{
    delete ui;
}

void ImageSettings::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void ImageSettings::applyImageSettings() const
{
    imagecapture->setFileFormat(boxValue(ui->imageCodecBox).value<QImageCapture::FileFormat>());
    imagecapture->setQuality(QImageCapture::Quality(ui->imageQualitySlider->value()));
    imagecapture->setResolution(boxValue(ui->imageResolutionBox).toSize());
}


QVariant ImageSettings::boxValue(const QComboBox *box) const
{
    int idx = box->currentIndex();
    if (idx == -1)
        return QVariant();

    return box->itemData(idx);
}

void ImageSettings::selectComboBoxItem(QComboBox *box, const QVariant &value)
{
    for (int i = 0; i < box->count(); ++i) {
        if (box->itemData(i) == value) {
            box->setCurrentIndex(i);
            break;
        }
    }
}

