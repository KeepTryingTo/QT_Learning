#ifndef IMAGESETTINGS_H
#define IMAGESETTINGS_H

#include <QDialog>
#include <QComboBox>
#include <QImageCapture>
#include <QMediaCaptureSession>

namespace Ui {
class ImageSettings;
}

class ImageSettings : public QDialog
{
    Q_OBJECT

public:
    // explicit ImageSettings(QWidget *parent = nullptr);
    explicit ImageSettings(QImageCapture *imageCapture, QWidget *parent = nullptr);
    ~ImageSettings();

    void applyImageSettings() const;

    QString format() const;
    void setFormat(const QString &format);

protected:
    void changeEvent(QEvent *e) override;

private:
    QVariant boxValue(const QComboBox *box) const;
    void selectComboBoxItem(QComboBox *box, const QVariant &value);

    Ui::ImageSettings *ui;
    QImageCapture *imagecapture;
};

#endif // IMAGESETTINGS_H
