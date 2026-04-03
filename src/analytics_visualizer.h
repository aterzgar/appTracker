#ifndef ANALYTICS_VISUALIZER_H
#define ANALYTICS_VISUALIZER_H

#include <QWidget>
#include <QPainter>
#include <vector>
#include <string>
#include <utility>

// ---------------------------------------------------------------------------
// PieChart
// ---------------------------------------------------------------------------
class PieChart : public QWidget {
    Q_OBJECT
public:
    struct DataPoint {
        QString label;
        int     value;
        QString color;
    };

    explicit PieChart(QWidget *parent = nullptr);

    void setData (const std::vector<DataPoint>& data);
    void setTitle(const QString& title);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Layout helpers – all values derived from widget size, never hardcoded
    int   titleHeight()     const;
    int   legendRowHeight() const;
    int   legendRows()      const;
    int   legendHeight()    const;
    QRect pieRect()         const;

    void drawPie   (QPainter&, const QRect&, int total,
                    const std::vector<int>& spans); // pre-computed spans
    void drawLegend(QPainter&, const QRect&, int total);

    // Compute span angles once so both drawing passes use identical values
    std::vector<int> computeSpans(int total) const;

    std::vector<DataPoint> data;
    QString                chartTitle;
};

// ---------------------------------------------------------------------------
// CartesianChart – shared base for BarChart and LineChart
//
// Owns the common layout constants, chartRect(), and the axes/grid drawing
// so neither subclass has to duplicate them.
// ---------------------------------------------------------------------------
class CartesianChart : public QWidget {
    Q_OBJECT
public:
    explicit CartesianChart(QWidget *parent = nullptr);

protected:
    // Returns the plotting area (inside axes), derived from widget size.
    QRect chartRect() const;

    // Draws grid lines, axis lines, and Y-axis tick labels.
    // Caller supplies the effective max value for the Y scale.
    void drawAxesAndGrid(QPainter&, const QRect&, int maxValue) const;
};

// ---------------------------------------------------------------------------
// BarChart
// ---------------------------------------------------------------------------
class BarChart : public CartesianChart {
    Q_OBJECT
public:
    struct Bar {
        QString label;
        int     value;
        QString color;
    };

    explicit BarChart(QWidget *parent = nullptr);

    void setData(const std::vector<Bar>& data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawBars(QPainter&, const QRect&);

    std::vector<Bar> data;
    int              maxValue = 0;
};

// ---------------------------------------------------------------------------
// LineChart
// ---------------------------------------------------------------------------
class LineChart : public CartesianChart {
    Q_OBJECT
public:
    struct Point {
        QString label;
        int     value;
    };

    explicit LineChart(QWidget *parent = nullptr);

    void setData(const std::vector<Point>& data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawLines(QPainter&, const QRect&);

    std::vector<Point> data;
    int                maxValue = 0;
};

// ---------------------------------------------------------------------------
// LabeledProgressBar
// ---------------------------------------------------------------------------
class LabeledProgressBar : public QWidget {
    Q_OBJECT
public:
    explicit LabeledProgressBar(const QString& label,
                                int current, int target,
                                const QString& color,
                                QWidget* parent = nullptr);

    void setValue(int current, int target);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // All geometry derived from widget size
    int   labelWidth() const;
    int   valueWidth() const;
    QRect barRect()    const;

    QString label;
    int     currentValue;
    int     targetValue;
    QString barColor;
};

#endif // ANALYTICS_VISUALIZER_H