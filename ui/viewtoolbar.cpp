#include "ui/viewtoolbar.h"
#include "ui/squeezedtextlabel.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QFont>
#include <QToolButton>

static int icnSize=-1;

int Ui::ViewToolBar::iconSize() {
    if (-1==icnSize) {
        icnSize=22; // TODO HiDPI ???
    }
    return icnSize;
}

Ui::ViewToolBar::ViewToolBar(QWidget *p)
    : QStackedWidget(p)
{
    title=new QLabel(this);
    QFont f=title->font();
    f.setBold(true);
    title->setFont(f);
    QStackedWidget::addWidget(title);
    QWidget *main=new QWidget(this);
    layout=new QHBoxLayout(main);
    layout->setMargin(0);
    layout->setSpacing(0);
    QStackedWidget::addWidget(main);
    showTitle(true);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
}

void Ui::ViewToolBar::showEvent(QShowEvent *ev) {
    int h=0;
    for (int p=0; p<count(); ++p) {
        if (widget(p)->height()>h) {
            h=widget(p)->height();
        }
    }
    setFixedHeight(h);
    QStackedWidget::showEvent(ev);
}

void Ui::ViewToolBar::setTitle(const QString &str) {
    title->setText("  "+str);
}

void Ui::ViewToolBar::addWidget(QWidget *w, bool left) {
    if (qobject_cast<QToolButton *>(w)) {
        static_cast<QToolButton *>(w)->setIconSize(QSize(icnSize, icnSize));
    }
    layout->insertWidget(left ? layout->count()-1 : layout->count(), w);
}

void Ui::ViewToolBar::addSpacer(int space, bool left) {
    layout->insertSpacerItem(left ? layout->count()-1 : layout->count(), new QSpacerItem(space, 0, QSizePolicy::Fixed, QSizePolicy::Fixed));
}
