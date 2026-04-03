#include "analytics_visualizer.h"
#include <QPainterPath>
#include <QFontMetrics>
#include <cmath>
#include <numeric>
#include <algorithm>

// Fallback for M_PI if not defined (some platforms)
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ============================================================================
// Shared layout constants  (ratios – never raw pixels)
// ============================================================================
namespace {

// Outer margin as a fraction of the widget's shorter side
constexpr double kMarginRatio       = 0.04;
constexpr int    kMinMargin         = 6;

// PieChart legend
constexpr double kLegendRowRatio    = 0.075;
constexpr int    kMinLegendRow      = 18;
constexpr int    kMaxLegendRow      = 26;
constexpr double kPieLegendGapRatio = 0.03;

// PieChart slices
constexpr double kExplosionRatio    = 0.04;
constexpr int    kExplosionMinPct   = 10;
constexpr double kTextMinDeg        = 12.0;
constexpr double kTextRadiusFrac    = 0.62;

// Cartesian charts (shared by BarChart and LineChart)
constexpr double kBarTopRatio       = 0.06;
constexpr double kBarBottomRatio    = 0.18;
constexpr double kBarLeftRatio      = 0.12;
constexpr double kBarRightRatio     = 0.04;
constexpr int    kGridLines         = 5;

// LabeledProgressBar
constexpr double kPBLabelFrac       = 0.30;
constexpr double kPBValueFrac       = 0.18;
// Vertical padding clamped so it never swallows the bar at extreme heights
constexpr int    kPBVPadMin         = 4;
constexpr int    kPBVPadMax         = 8;

// ---- helpers ----

inline int scaled(int base, double ratio, int minVal) {
    return std::max(minVal, static_cast<int>(base * ratio));
}

// Perceived luminance (ITU-R BT.601).  Returns 0–255.
inline int luminance(const QColor& c) {
    return (c.red() * 299 + c.green() * 587 + c.blue() * 114) / 1000;
}

// Chooses white or black for maximum contrast against `bg`.
inline QColor contrastText(const QColor& bg) {
    return luminance(bg) < 130 ? Qt::white : Qt::black;
}

} // anonymous namespace


// ============================================================================
// PieChart – geometry helpers
// ============================================================================

int PieChart::titleHeight() const {
    if (chartTitle.isEmpty()) return 0;
    return scaled(height(), 0.09, 24);
}

int PieChart::legendRowHeight() const {
    return std::min(scaled(height(), kLegendRowRatio, kMinLegendRow), kMaxLegendRow);
}

int PieChart::legendRows() const {
    if (data.empty()) return 0;
    int cols = (static_cast<int>(data.size()) <= 3) ? static_cast<int>(data.size()) : 2;
    return static_cast<int>((data.size() + cols - 1) / cols);
}

int PieChart::legendHeight() const {
    int rh  = legendRowHeight();
    int gap = rh / 3;
    return legendRows() * (rh + gap) + gap;
}

QRect PieChart::pieRect() const {
    int m     = scaled(std::min(width(), height()), kMarginRatio, kMinMargin);
    int tH    = titleHeight();
    int lH    = legendHeight();
    int lgGap = scaled(height(), kPieLegendGapRatio, 4);

    int left   = m;
    int top    = m + tH;
    int right  = width()  - m;
    int bottom = height() - m - lH - lgGap;

    if (right - left < 60 || bottom - top < 60)
        return QRect(m, m + tH, std::max(60, right - left), std::max(60, bottom - top));

    return QRect(left, top, right - left, bottom - top);
}

// Pre-compute span angles once so both drawing passes are guaranteed identical.
std::vector<int> PieChart::computeSpans(int total) const {
    std::vector<int> spans;
    spans.reserve(data.size());
    int accumulated = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        int span;
        if (i == data.size() - 1) {
            // Last slice: fill whatever is left to avoid a gap caused by rounding
            span = 5760 - accumulated;
        } else {
            span = static_cast<int>(
                static_cast<double>(data[i].value) / total * 5760.0 + 0.5);
        }
        spans.push_back(span);
        accumulated += span;
    }
    return spans;
}

// ============================================================================
// PieChart – paint
// ============================================================================

PieChart::PieChart(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(400);
    setMinimumWidth(400);
}

void PieChart::setData(const std::vector<DataPoint>& newData) {
    data = newData;
    update();
}

void PieChart::setTitle(const QString& title) {
    chartTitle = title;
    update();
}

void PieChart::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);

    // Title – use palette so it works on dark themes
    if (!chartTitle.isEmpty()) {
        int tH = titleHeight();
        painter.setFont(QFont("Arial", std::max(9, tH / 2), QFont::Bold));
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(0, 0, width(), tH, Qt::AlignCenter, chartTitle);
    }

    if (data.empty()) {
        painter.setFont(QFont("Arial", 11));
        painter.setPen(palette().color(QPalette::PlaceholderText));
        painter.drawText(rect(), Qt::AlignCenter, "No data available");
        return;
    }

    int total = 0;
    for (const auto& p : data) total += p.value;
    if (total == 0) return;

    // Compute spans once; pass to both drawing helpers
    const auto spans = computeSpans(total);

    QRect pr = pieRect();
    drawPie(painter, pr, total, spans);

    int m     = scaled(std::min(width(), height()), kMarginRatio, kMinMargin);
    int lgGap = scaled(height(), kPieLegendGapRatio, 4);
    drawLegend(painter, QRect(m, pr.bottom() + lgGap, width() - 2 * m, legendHeight()), total);
}

void PieChart::drawPie(QPainter& painter, const QRect& rect, int total,
                       const std::vector<int>& spans) {
    // Square, centered inside the available rect - use QRectF for sub-pixel precision
    int side = std::min(rect.width(), rect.height());
    QRectF sq(rect.left() + (rect.width()  - side) / 2.0,
              rect.top()  + (rect.height() - side) / 2.0,
              static_cast<qreal>(side), static_cast<qreal>(side));

    qreal explosion = static_cast<qreal>(scaled(side / 2, kExplosionRatio, 2));
    qreal radius = side / 2.0;

    // Find largest slice to potentially explode
    int explodedIndex = -1;
    int maxValue = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        if (data[i].value > maxValue) {
            maxValue = data[i].value;
            explodedIndex = static_cast<int>(i);
        }
    }

    // ---- Pass 1: draw slices (use thin white pen for natural separators) ----
    int startAngle = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        const int spanAngle = spans[i];
        const double pct    = static_cast<double>(data[i].value) * 100.0 / total;

        painter.save();

        // Handle explosion translation (Qt Y-axis grows downward so invert dy)
        if (static_cast<int>(i) == explodedIndex && pct >= kExplosionMinPct) {
            qreal midRad = (startAngle + spanAngle / 2.0) / 16.0 * M_PI / 180.0;
            qreal dx = explosion * std::cos(midRad);
            qreal dy = -explosion * std::sin(midRad);
            painter.translate(dx, dy);
        }

        QColor base(data[i].color);
        QRadialGradient grad(sq.center(), radius);
        grad.setColorAt(0, base.lighter(120));
        grad.setColorAt(0.8, base);
        grad.setColorAt(1, base.darker(130));

        painter.setBrush(grad);
        painter.setPen(QPen(Qt::white, 1.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPie(sq, startAngle, spanAngle);

        painter.restore();
        startAngle += spanAngle;
    }

    // ---- Pass 2: percentage labels (positioned with correct Y-sign and explosion offset) ----
    startAngle = 0;
    for (std::size_t i = 0; i < data.size(); ++i) {
        const int  spanAngle = spans[i];
        const double pct     = static_cast<double>(data[i].value) * 100.0 / total;
        const double spanDeg = spanAngle / 16.0;

        if (spanDeg >= kTextMinDeg && pct >= 4.0) {
            qreal midRad = (startAngle + spanAngle / 2.0) / 16.0 * M_PI / 180.0;
            qreal r      = radius * kTextRadiusFrac;

            // If exploded, push the label outward by the same amount
            qreal labelExplosion = 0;
            if (static_cast<int>(i) == explodedIndex && pct >= kExplosionMinPct) {
                labelExplosion = explosion;
            }

            qreal cx = sq.center().x() + (r + labelExplosion) * std::cos(midRad);
            qreal cy = sq.center().y() - (r + labelExplosion) * std::sin(midRad);

            int fontSize = std::max(7, side / 22);
            QFont f("Arial", fontSize, QFont::Bold);
            painter.setFont(f);
            QFontMetrics fm(f);

            QString txt = QString::number(static_cast<int>(pct + 0.5)) + "%";
            int tw = fm.horizontalAdvance(txt) + 4;
            int th = fm.height() + 2;

            int tx = std::clamp(static_cast<int>(cx - tw / 2),
                                static_cast<int>(std::lround(sq.left())),
                                static_cast<int>(std::lround(sq.right() - tw)));
            int ty = std::clamp(static_cast<int>(cy - th / 2),
                                static_cast<int>(std::lround(sq.top())),
                                static_cast<int>(std::lround(sq.bottom() - th)));

            painter.setPen(contrastText(QColor(data[i].color)));
            painter.drawText(tx, ty, tw, th, Qt::AlignCenter, txt);
        }

        startAngle += spanAngle;
    }
}

void PieChart::drawLegend(QPainter& painter, const QRect& area, int total) {
    if (data.empty()) return;

    int rh       = legendRowHeight();
    int gap      = rh / 3;
    int cols     = (static_cast<int>(data.size()) <= 3) ? static_cast<int>(data.size()) : 2;
    int colW     = area.width() / cols;
    int swSize   = std::max(8, rh - 4);
    int fontSize = std::max(7, rh - 6);

    QFont f("Arial", fontSize);
    painter.setFont(f);
    QFontMetrics fm(f);

    for (int i = 0; i < static_cast<int>(data.size()); ++i) {
        int col = i % cols;
        int row = i / cols;
        int x   = area.left() + col * colW;
        int y   = area.top()  + row * (rh + gap);

        // Color swatch
        QColor c(data[i].color);
        painter.setPen(QPen(c.darker(130), 1));
        painter.setBrush(c);
        painter.drawRect(x, y + (rh - swSize) / 2, swSize, swSize);

        // Label – elide if too wide; use palette text colour for theme support
        int pct     = static_cast<int>(static_cast<double>(data[i].value) / total * 100.0 + 0.5);
        QString lbl = QString("%1: %2 (%3%)")
                        .arg(data[i].label).arg(data[i].value).arg(pct);
        int textX   = x + swSize + 4;
        int textW   = colW - swSize - 8;
        lbl         = fm.elidedText(lbl, Qt::ElideRight, textW);

        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(textX, y, textW, rh, Qt::AlignLeft | Qt::AlignVCenter, lbl);
    }
}


// ============================================================================
// CartesianChart – shared base
// ============================================================================

CartesianChart::CartesianChart(QWidget *parent) : QWidget(parent) {
    setMinimumHeight(200);
    setMinimumWidth(200);
}

QRect CartesianChart::chartRect() const {
    int left   = scaled(width(),  kBarLeftRatio,   30);
    int top    = scaled(height(), kBarTopRatio,     10);
    int right  = width()  - scaled(width(),  kBarRightRatio,  8);
    int bottom = height() - scaled(height(), kBarBottomRatio, 36);
    return QRect(left, top, right - left, bottom - top);
}

void CartesianChart::drawAxesAndGrid(QPainter& painter, const QRect& r, int maxValue) const {
    const int safeMax = (maxValue == 0) ? 1 : maxValue;

    int fontSize = std::max(7, scaled(height(), 0.04, 8));
    QFont f("Arial", fontSize);
    painter.setFont(f);
    QFontMetrics fm(f);

    // Grid lines
    QColor gridColor = palette().color(QPalette::Mid);
    painter.setPen(QPen(gridColor, 1));
    for (int i = 0; i <= kGridLines; ++i) {
        int y = r.bottom() - (i * r.height()) / kGridLines;
        painter.drawLine(r.left(), y, r.right(), y);
    }

    // Axis lines – use WindowText so they're visible on any theme
    QColor axisColor = palette().color(QPalette::WindowText);
    painter.setPen(QPen(axisColor, 1));
    painter.drawLine(r.left(), r.bottom(), r.right(), r.bottom()); // X-axis
    painter.drawLine(r.left(), r.top(),    r.left(),  r.bottom()); // Y-axis

    // Y-axis tick labels
    for (int i = 0; i <= kGridLines; ++i) {
        int y   = r.bottom() - (i * r.height()) / kGridLines;
        int val = (i * safeMax) / kGridLines;
        QString txt = QString::number(val);
        int tw  = fm.horizontalAdvance(txt);
        painter.setPen(axisColor);
        painter.drawText(r.left() - tw - 4, y - fm.height() / 2,
                         tw, fm.height(), Qt::AlignRight | Qt::AlignVCenter, txt);
    }
}


// ============================================================================
// BarChart
// ============================================================================

BarChart::BarChart(QWidget *parent) : CartesianChart(parent) {}

void BarChart::setData(const std::vector<Bar>& newData) {
    data     = newData;
    maxValue = 0;
    for (const auto& b : data) maxValue = std::max(maxValue, b.value);
    update();
}

void BarChart::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (data.empty()) {
        painter.drawText(rect(), Qt::AlignCenter, "No data available");
        return;
    }

    QRect cr = chartRect();
    drawAxesAndGrid(painter, cr, maxValue);
    drawBars(painter, cr);

    // Y-axis side label
    int fontSize = std::max(7, scaled(width(), 0.025, 8));
    QFont labelFont("Arial", fontSize);
    painter.setFont(labelFont);
    painter.setPen(palette().color(QPalette::WindowText));
    painter.save();
    painter.translate(scaled(width(), 0.02, 4), height() / 2);
    painter.rotate(-90);

    // Measure text width and draw without restrictive bounding box
    QFontMetrics fm(labelFont);
    int textWidth = fm.horizontalAdvance("Count");
    painter.drawText(-textWidth / 2, fontSize / 2, "Count");

    painter.restore();
}

void BarChart::drawBars(QPainter& painter, const QRect& r) {
    if (maxValue == 0) return;

    const int    n      = static_cast<int>(data.size());
    const double slotW  = static_cast<double>(r.width()) / n;
    const double barW   = slotW * 0.60;
    const double halfGap= slotW * 0.20;

    const int fontSize  = std::max(7, scaled(height(), 0.038, 8));
    QFont     f    ("Arial", fontSize);
    QFont     boldF("Arial", fontSize, QFont::Bold);
    QFontMetrics fm   (f);
    QFontMetrics fmBold(boldF); // FIX: separate metrics for bold font

    const QColor textColor = palette().color(QPalette::WindowText); // FIX: palette-aware

    for (int i = 0; i < n; ++i) {
        int barX = static_cast<int>(r.left() + i * slotW + halfGap);
        int bW   = static_cast<int>(barW);
        int bH   = (data[i].value * r.height()) / maxValue;
        int barY = r.bottom() - bH;

        // Bar fill
        QColor base(data[i].color);
        QLinearGradient grad(barX, barY, barX, r.bottom());
        grad.setColorAt(0, base.lighter(115));
        grad.setColorAt(1, base.darker(115));
        painter.setBrush(grad);
        painter.setPen(QPen(base.darker(130), 1));
        painter.drawRect(barX, barY, bW, bH);

        // Value label above bar – use fmBold for correct height measurement
        painter.setFont(boldF);
        painter.setPen(textColor);
        QString val = QString::number(data[i].value);
        int gap = std::max(2, scaled(height(), 0.012, 2));
        painter.drawText(barX, barY - fmBold.height() - gap,
                         bW,   fmBold.height(),
                         Qt::AlignCenter, val);

        // X-axis label – palette text colour, elide if too wide
        painter.setFont(f);
        painter.setPen(textColor);
        int labelY = r.bottom() + std::max(2, scaled(height(), 0.01, 2));
        int labelH = std::max(fm.height(), height() - labelY - 2);
        QString lbl = fm.elidedText(data[i].label, Qt::ElideRight,
                                    static_cast<int>(slotW - 2));
        painter.drawText(static_cast<int>(r.left() + i * slotW), labelY,
                         static_cast<int>(slotW), labelH,
                         Qt::AlignHCenter | Qt::AlignTop, lbl);
    }
}


// ============================================================================
// LineChart
// ============================================================================

LineChart::LineChart(QWidget *parent) : CartesianChart(parent) {}

void LineChart::setData(const std::vector<Point>& newData) {
    data     = newData;
    maxValue = 0;
    for (const auto& p : data) maxValue = std::max(maxValue, p.value);
    update();
}

void LineChart::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (data.size() < 2) {
        painter.drawText(rect(), Qt::AlignCenter, "Insufficient data for trend line");
        return;
    }

    QRect cr = chartRect();
    drawAxesAndGrid(painter, cr, maxValue);
    drawLines(painter, cr);
}

void LineChart::drawLines(QPainter& painter, const QRect& r) {
    const int safeMax   = (maxValue == 0) ? 1 : maxValue;
    const int n         = static_cast<int>(data.size());
    const int dotRadius = std::max(3, scaled(std::min(width(), height()), 0.012, 3));
    const int fontSize  = std::max(7, scaled(height(), 0.038, 8));

    // Build pixel points
    std::vector<QPointF> pts(n);
    for (int i = 0; i < n; ++i) {
        double x = r.left() + static_cast<double>(i) / (n - 1) * r.width();
        double y = r.bottom() - static_cast<double>(data[i].value) / safeMax * r.height();
        pts[i]   = QPointF(x, y);
    }

    // Filled area under the line
    QPainterPath area;
    area.moveTo(pts[0].x(), r.bottom());
    for (const auto& p : pts) area.lineTo(p);
    area.lineTo(pts.back().x(), r.bottom());
    area.closeSubpath();

    QLinearGradient areaGrad(0, r.top(), 0, r.bottom());
    areaGrad.setColorAt(0, QColor(52, 152, 219, 80));
    areaGrad.setColorAt(1, QColor(52, 152, 219, 10));
    painter.setBrush(areaGrad);
    painter.setPen(Qt::NoPen);
    painter.drawPath(area);

    // Line
    int lw = std::max(1, scaled(std::min(width(), height()), 0.005, 2));
    painter.setPen(QPen(QColor("#2980b9"), lw));
    painter.setBrush(Qt::NoBrush);
    for (int i = 0; i < n - 1; ++i)
        painter.drawLine(pts[i], pts[i + 1]);

    // Dots + X-axis labels
    QFont f("Arial", fontSize);
    QFontMetrics fm(f);
    // Slot width between adjacent points (or full width for n==1, guarded above)
    const double slotW = static_cast<double>(r.width()) / (n - 1);

    const QColor textColor = palette().color(QPalette::WindowText); // FIX: palette-aware

    for (int i = 0; i < n; ++i) {
        // Dot
        painter.setBrush(QColor("#2980b9"));
        painter.setPen(QPen(Qt::white, std::max(1, dotRadius / 2)));
        painter.drawEllipse(pts[i], dotRadius, dotRadius);

        // X-axis label
        // FIX: all labels use the same coordinate reference (r) and are clamped to [r.left(), r.right()]
        painter.setFont(f);
        painter.setPen(textColor);

        int labelY = r.bottom() + std::max(2, scaled(height(), 0.01, 2));
        int labelH = std::max(fm.height(), height() - labelY - 2);
        int lw2    = static_cast<int>(slotW);
        // Centre the label slot on the point, then clamp so it never leaves the chart rect
        int lx = static_cast<int>(pts[i].x() - slotW / 2.0);
        lx = std::clamp(lx, r.left(), r.right() - lw2);

        QString lbl = fm.elidedText(data[i].label, Qt::ElideRight, lw2 - 2);
        painter.drawText(lx, labelY, lw2, labelH, Qt::AlignHCenter | Qt::AlignTop, lbl);
    }
}


// ============================================================================
// LabeledProgressBar – geometry helpers
// ============================================================================

int LabeledProgressBar::labelWidth() const {
    return static_cast<int>(width() * kPBLabelFrac);
}

int LabeledProgressBar::valueWidth() const {
    return static_cast<int>(width() * kPBValueFrac);
}

QRect LabeledProgressBar::barRect() const {
    // FIX: clamp vPad so the bar is never too thin or too thick
    int vPad = std::clamp(static_cast<int>(height() * 0.22), kPBVPadMin, kPBVPadMax);
    int lw   = labelWidth();
    int vw   = valueWidth();
    int bw   = width() - lw - vw;
    return QRect(lw, vPad, std::max(20, bw), height() - 2 * vPad);
}

// ============================================================================
// LabeledProgressBar – paint
// ============================================================================

LabeledProgressBar::LabeledProgressBar(const QString& lbl, int cur, int tgt,
                                       const QString& color, QWidget* parent)
    : QWidget(parent), label(lbl), currentValue(cur), targetValue(tgt), barColor(color)
{
    setMinimumHeight(24);
    setMaximumHeight(40);
}

void LabeledProgressBar::setValue(int cur, int tgt) {
    currentValue = cur;
    targetValue  = tgt;
    update();
}

void LabeledProgressBar::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int fontSize = std::max(7, scaled(height(), 0.45, 8));
    QFont     f    ("Arial", fontSize);
    QFont     boldF("Arial", fontSize, QFont::Bold);
    QFontMetrics fm(f);
    painter.setFont(f);

    // FIX: label sits on the widget background, so use palette WindowText –
    //      not a luminance calculation against the bar fill colour.
    const QColor textColor = palette().color(QPalette::WindowText);

    // Label (elided if needed)
    int lw = labelWidth();
    QString elidedLabel = fm.elidedText(label, Qt::ElideRight, lw - 4);
    painter.setPen(textColor);
    painter.drawText(0, 0, lw - 4, height(), Qt::AlignLeft | Qt::AlignVCenter, elidedLabel);

    // Track
    QRect br = barRect();
    painter.setPen(QPen(palette().color(QPalette::Mid), 1));
    painter.setBrush(palette().color(QPalette::Base));
    painter.drawRoundedRect(br, br.height() / 2.0, br.height() / 2.0);

    // Fill
    if (targetValue > 0 && currentValue > 0) {
        double ratio = std::min(1.0, static_cast<double>(currentValue) / targetValue);
        int fillW    = static_cast<int>(br.width() * ratio);
        if (fillW > 0) {
            QRect fillRect(br.left(), br.top(), fillW, br.height());
            QColor base(barColor);
            QLinearGradient grad(fillRect.topLeft(), fillRect.topRight());
            grad.setColorAt(0, base.lighter(120));
            grad.setColorAt(1, base);

            painter.setPen(Qt::NoPen);
            painter.setBrush(grad);
            // Intersect fill with the rounded track so corners never overflow
            QPainterPath fillPath, trackPath;
            fillPath.addRoundedRect(fillRect, br.height() / 2.0, br.height() / 2.0);
            trackPath.addRoundedRect(br,       br.height() / 2.0, br.height() / 2.0);
            painter.drawPath(fillPath.intersected(trackPath));
        }
    }

    // Value text to the right of the bar
    int vw = valueWidth();
    int vx = br.right() + 4;
    painter.setPen(textColor);
    painter.setFont(boldF);
    QString valTxt = QString("%1/%2").arg(currentValue).arg(targetValue);
    painter.drawText(vx, 0, vw - 4, height(), Qt::AlignLeft | Qt::AlignVCenter, valTxt);
}