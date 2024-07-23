#ifndef VIDEOSETTINGS_H
#define VIDEOSETTINGS_H

#include <QDialog>
#include <QComboBox>
#include <QCamera>
#include <QMediaRecorder>

namespace Ui {
class VideoSettings;
}

class VideoSettings : public QDialog
{
    Q_OBJECT

public:
    // explicit VideoSettings(QWidget *parent = nullptr);
    explicit VideoSettings(QMediaRecorder *mediaRecorder, QWidget *parent = nullptr);
    ~VideoSettings();

    void applySettings();
    void updateFormatsAndCodecs();

protected:
    void changeEvent(QEvent *e) override;

private:
    QVariant boxValue(const QComboBox*) const;
    void selectComboBoxItem(QComboBox *box, const QVariant &value);

    Ui::VideoSettings *ui;
    QMediaRecorder * mediaRecorder;
    bool m_updatingFormats = false;
};

#endif // VIDEOSETTINGS_H
