#ifndef AUDIOWAVEFORMWIDGET_H
#define AUDIOWAVEFORMWIDGET_H

#include <QWidget>
#include <QVector>
#include <QPainter>
#include <QPaintEvent>

class AudioWaveformWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AudioWaveformWidget(QWidget *parent = nullptr);
    void setWaveformData(const QVector<qreal> &data, qint64 duration);
    void setCurrentPosition(qint64 position);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<qreal> m_waveformData;
    qint64 m_totalDuration; // Total duration of the audio in milliseconds
    qint64 m_currentPosition; // Current playback position in milliseconds
};

#endif // AUDIOWAVEFORMWIDGET_H
