// TimelineLegend.h


#pragma once
#include <QWidget>
#include <QString>
#include <QColor>
#include <QVector>


/**
 * @class TimelineLegend
 * @brief A QWidget overlay that displays event type to color mapping
 *
 * Features:
 * - Displays color swatches with event type labels
 * - Positioned as overlay on top of TimelineView
 * - Stays fixed to viewport (doesn't scroll with timeline)
 * - Semi-transparent background
 * - Automatically updates when view is resized or splitter moves
 */
class TimelineLegend : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief Struct representing a single legend entry
     */
    struct LegendEntry
    {
        QString label;      ///< Event type name
        QColor color;       ///< Associated color
    };

    /**
     * @brief Constructs a TimelineLegend widget
     * @param parent Parent widget (should be TimelineView or TimelineModule)
     */
    explicit TimelineLegend(QWidget* parent = nullptr);

    /**
     * @brief Set the legend entries
     * @param entries Vector of legend entries to display
     */
    void setEntries(const QVector<LegendEntry>& entries);

    /**
     * @brief Get the recommended size for the legend
     */
    QSize sizeHint() const override;

protected:
    /**
     * @brief Paint the legend
     */
    void paintEvent(QPaintEvent* event) override;

private:
    /**
     * @brief Calculate the size needed for the legend
     */
    void calculateSize();

    QVector<LegendEntry> entries_;              ///< Legend entries to display

    // Layout constants
    static constexpr int PADDING = 10;              ///< Padding around the legend
    static constexpr int SWATCH_SIZE = 20;          ///< Size of color swatch
    static constexpr int LABEL_SPACING = 8;         ///< Space between swatch and label
    static constexpr int ENTRY_SPACING = 6;         ///< Vertical space between entries
    static constexpr int BORDER_WIDTH = 2;          ///< Border thickness
};
