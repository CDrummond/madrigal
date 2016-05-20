#include "ui/viewtoolbar.h"
#include "ui/squeezedtextlabel.h"
#include "ui/utils.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QFont>
#include <QToolButton>
#include <QApplication>

int Ui::ViewToolBar::iconSize() {
    static int size=-1;
    if (-1==size) {
        size=Utils::scaleForDpi(20);
    }
    return size;
}

int Ui::ViewToolBar::buttonSize() {
    static int size=-1;
    if (-1==size) {
        int iSize=iconSize();
        size=qMax(qApp->fontMetrics().height(), iSize)+Utils::scaleForDpi(8);
    }
    return size;
}

Ui::ViewToolBar::ViewToolBar(QWidget *p)
    : QStackedWidget(p)
{
    title=new QLabel(this);
//    QFont f=title->font();
//    f.setBold(true);
//    title->setFont(f);
    QStackedWidget::addWidget(title);
    QWidget *main=new QWidget(this);
    layout=new QHBoxLayout(main);
    layout->setMargin(0);
    layout->setSpacing(0);
    QStackedWidget::addWidget(main);
    showTitle(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    setFixedHeight(buttonSize());
}

static bool isDark(const QColor &col) {
    return col.red()<64 && col.green()<64 && col.blue()<64;
}

void Ui::ViewToolBar::paintEvent(QPaintEvent *ev) {
    QPainter p(this);
    QColor col(isDark(palette().color(QPalette::Window)) ? Qt::white : Qt::black);
    col.setAlphaF(0.06);
    p.fillRect(rect(), col);
    QStackedWidget::paintEvent(ev);
}

void Ui::ViewToolBar::setTitle(const QString &str) {
    title->setText("  "+str);
}

void Ui::ViewToolBar::addWidget(QWidget *w, bool left) {
    if (qobject_cast<QToolButton *>(w)) {
        QToolButton *tb=static_cast<QToolButton *>(w);
        int iSize=iconSize();
        tb->setIconSize(QSize(iSize, iSize));
        if (Qt::ToolButtonTextBesideIcon==tb->toolButtonStyle()) {
            tb->setFixedHeight(buttonSize());
        } else {
            int bSize=buttonSize();
            tb->setFixedSize(QSize(bSize, bSize));
        }
    }
    layout->insertWidget(left ? layout->count()-1 : layout->count(), w);
}

void Ui::ViewToolBar::addSpacer(int space, bool left) {
    layout->insertSpacerItem(left ? layout->count()-1 : layout->count(), new QSpacerItem(space, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
}
