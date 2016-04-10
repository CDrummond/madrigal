/***************************************************************************
 *   Copyright (C) 2005-09 by the Quassel Project                          *
 *   devel@quassel-irc.org                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************
 * Parts of this implementation are taken from KDE's kaction.cpp           *
 ***************************************************************************/

#include "ui/action.h"
#include "ui/gtkstyle.h"
#include "core/utils.h"
#include <QApplication>
#include <QKeySequence>

static const char * constConfigurableProp="shortcut-configurable";
static const char *constPlainToolTipProperty="plain-tt";

Ui::Action::Action(QObject *parent)
    : QAction(parent)
{
    init();
}

Ui::Action::Action(const QString &text, QObject *parent, const QObject *receiver, const char *slot, const QKeySequence &shortcut)
    : QAction(parent)
{
    init();
    setText(text);
    setShortcut(shortcut);
    if (receiver && slot) {
        connect(this, SIGNAL(triggered()), receiver, slot);
    }
}

Ui::Action::Action(const QIcon &icon, const QString &text, QObject *parent, const QObject *receiver, const char *slot, const QKeySequence &shortcut)
    : QAction(parent)
{
    init();
    setIcon(icon);
    setText(text);
    setShortcut(shortcut);
    if (receiver && slot) {
        connect(this, SIGNAL(triggered()), receiver, slot);
    }
}

void Ui::Action::initIcon(QAction *act) {
    if (GtkStyle::isActive() && act) {
        act->setIconVisibleInMenu(false);
    }
}

void Ui::Action::updateToolTip(QAction *act) {
    if (!act) {
        return;
    }
    QKeySequence sc=act->shortcut();
    if (sc.isEmpty()) {
        act->setToolTip(act->property(constPlainToolTipProperty).toString());
        act->setProperty(constPlainToolTipProperty, QString());
    } else {
        QString tt=act->property(constPlainToolTipProperty).toString();
        if (tt.isEmpty()) {
            tt=act->toolTip();
            act->setProperty(constPlainToolTipProperty, tt);
        }
        act->setToolTip(QString::fromLatin1("%1 <span style=\"color: gray; font-size: small\">%2</span>")
                        .arg(tt)
                        .arg(sc.toString(QKeySequence::NativeText)));
    }
}

const char * Ui::Action::constTtForSettings="tt-for-settings";

QString Ui::Action::settingsText(QAction *act) {
    if (act->property(constTtForSettings).toBool()) {
        QString tt=Core::Utils::stripAcceleratorMarkers(act->property(constPlainToolTipProperty).toString());
        if (tt.isEmpty()) {
            tt=Core::Utils::stripAcceleratorMarkers(act->toolTip());
        }
        if (!tt.isEmpty()) {
            return tt;
        }
    }
    return Core::Utils::stripAcceleratorMarkers(act->text());
}

void Ui::Action::init() {
    setProperty(constConfigurableProp, true);
}

bool Ui::Action::isShortcutConfigurable() const {
    return property(constConfigurableProp).toBool();
}

void Ui::Action::setShortcutConfigurable(bool b) {
    setProperty(constConfigurableProp, b);
}

QKeySequence Ui::Action::shortcut(ShortcutTypes type) const {
    Q_ASSERT(type);
    if (DefaultShortcut==type) {
        return property("defaultShortcut").value<QKeySequence>();
    }

    if(shortcuts().count()) {
        return shortcuts().value(0);
    }
    return QKeySequence();
}

void Ui::Action::setShortcut(const QShortcut &shortcut, ShortcutTypes type) {
    setShortcut(shortcut.key(), type);
}

void Ui::Action::setShortcut(const QKeySequence &key, ShortcutTypes type) {
    Q_ASSERT(type);

    if(type & DefaultShortcut) {
        setProperty("defaultShortcut", key);
    }

    if(type & ActiveShortcut) {
        QAction::setShortcut(key);
    }
    updateToolTip(this);
}
