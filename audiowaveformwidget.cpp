#include "audiowaveformwidget.h"
#include <QDebug>
#include <QBrush>
#include <QPen>

AudioWaveformWidget::AudioWaveformWidget(QWidget *parent)
    : QWidget(parent),
    m_totalDuration(0),
    m_currentPosition(0)
{
    // Set a minimum size policy to allow it to expand in layouts
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    // Set a background for visibility
    setStyleSheet("background-color: #2b2b2b;"); // Dark background for the waveform
}

void AudioWaveformWidget::setWaveformData(const QVector<qreal> &data, qint64 duration)
{
    m_waveformData = data;
    m_totalDuration = duration;
    update(); // Request a repaint
}

void AudioWaveformWidget::setCurrentPosition(qint64 position)
{
    if (position != m_currentPosition) {
        m_currentPosition = position;
        update(); // Request a repaint
    }
}

void AudioWaveformWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event); // Suppress unused parameter warning
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Fill background
    painter.fillRect(rect(), QColor("#2b2b2b")); // Match stylesheet background

    if (m_waveformData.isEmpty() || m_totalDuration == 0) {
        painter.setPen(Qt::white);
        painter.drawText(rect(), Qt::AlignCenter, "Load an audio file for waveform");
        return;
    }

    // Waveform drawing
    painter.setPen(QPen(QColor("#00aaff"), 1)); // Blue waveform color
    painter.setBrush(QColor(0, 170, 255, 80)); // Semi-transparent blue fill

    qreal halfHeight = height() / 2.0;
    qreal scaleX = (qreal)width() / m_waveformData.size(); // Scale data points to widget width

    QPolygonF waveformPolygon;
    waveformPolygon.append(QPointF(0, halfHeight)); // Start at the left middle

    // Top half of the waveform
    for (int i = 0; i < m_waveformData.size(); ++i) {
        qreal x = i * scaleX;
        qreal y = halfHeight - (m_waveformData[i] * halfHeight); // Scale magnitude to half height
        waveformPolygon.append(QPointF(x, y));
    }

    // Bottom half of the waveform (mirror of the top)
    for (int i = m_waveformData.size() - 1; i >= 0; --i) {
        qreal x = i * scaleX;
        qreal y = halfHeight + (m_waveformData[i] * halfHeight); // Scale magnitude to half height
        waveformPolygon.append(QPointF(x, y));
    }
    waveformPolygon.append(QPointF(width(), halfHeight)); // End at the right middle

    painter.drawPolygon(waveformPolygon);


    // Draw playback position line
    if (m_totalDuration > 0 && m_currentPosition >= 0) {
        qreal positionX = (qreal)m_currentPosition / m_totalDuration * width();
        painter.setPen(QPen(Qt::red, 2)); // Red line for current position
        painter.drawLine(QPointF(positionX, 0), QPointF(positionX, height()));
    }
}
