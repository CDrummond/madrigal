/*
 * Madrigal
 *
 * Copyright (c) 2016 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "ui/preferencesdialog.h"
#include "ui/utils.h"
#include "ui/rendererview.h"
#include "core/images.h"
#ifdef QT_QTDBUS_FOUND
#include "dbus/notify.h"
#endif
#ifdef Q_OS_MAC
#include "mac/notify.h"
#endif
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>

Ui::PreferencesDialog::PreferencesDialog(QWidget *p, RendererView *rv)
    : QDialog(p)
    , rendererView(rv)
{
    setWindowTitle(tr("Preferences"));
    QVBoxLayout *lay=new QVBoxLayout(this);

    QGroupBox *behaviour=new QGroupBox(tr("Behaviour"), this);
    QVBoxLayout *behaviourLayout=new QVBoxLayout(behaviour);
    #if defined QT_QTDBUS_FOUND || defined Q_OS_MAC
    showNotifications=new QCheckBox(tr("Show notification when track changes"), behaviour);
    behaviourLayout->addWidget(showNotifications);
    #ifdef QT_QTDBUS_FOUND
    showNotifications->setChecked(Dbus::Notify::self()->isEnabled());
    #else
    showNotifications->setChecked(Mac::Notify::self()->isEnabled());
    #endif
    #endif
    autoScrollPlayQueue=new QCheckBox(tr("Center play queue on current track"), behaviour);
    behaviourLayout->addWidget(autoScrollPlayQueue);
    autoScrollPlayQueue->setChecked(rendererView->scrollQueue());

    QGroupBox *covers=new QGroupBox(tr("Album Covers"), this);
    QVBoxLayout *coversLayout=new QVBoxLayout(covers);
    QPushButton *clear=new QPushButton(tr("Clear Cache"), covers);
    clear->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    connect(clear, SIGNAL(clicked(bool)), this, SLOT(clearCache()));
    QLabel *coverNote=new QLabel(tr("%1 caches album covers from your media collection to disk. "
                                    "If you have updated a cover on your server, you may need to "
                                    "clear this cache in order for changes to be seen.").arg(PACKAGE_NAME_CASE), covers);
    coverNote->setWordWrap(true);
    coversLayout->addWidget(coverNote);
    coversLayout->addWidget(clear);

    lay->addWidget(behaviour);
    lay->addWidget(covers);
    lay->addItem(new QSpacerItem(0, 0, QSizePolicy::Preferred, QSizePolicy::MinimumExpanding));

    box=new QDialogButtonBox(this);
    #if defined Q_OS_MAC
    box->setStandardButtons(QDialogButtonBox::Close);
    box->button(QDialogButtonBox::Close)->setFocus();
    #elif defined Q_OS_WIN
    box->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);
    box->button(QDialogButtonBox::Cancel)->setFocus();
    #else
    if (Utils::KDE==Utils::currentDe()) {
        box->setStandardButtons(QDialogButtonBox::Ok|QDialogButtonBox::Apply|QDialogButtonBox::Cancel);
        box->button(QDialogButtonBox::Cancel)->setFocus();
        connect(box, SIGNAL(accepted()), this, SLOT(save()));
    } else {
        box->setStandardButtons(QDialogButtonBox::Close);
        box->button(QDialogButtonBox::Close)->setFocus();
    }
    #endif
    connect(box, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
    lay->addWidget(box);
    setMinimumWidth(480);
    if (box->button(QDialogButtonBox::Close)) {
        // Instant apply
        #ifdef QT_QTDBUS_FOUND
        connect(showNotifications, SIGNAL(toggled(bool)), this, SLOT(save()));
        #endif
        connect(autoScrollPlayQueue, SIGNAL(toggled(bool)), this, SLOT(save()));
    }
}

void Ui::PreferencesDialog::clearCache() {
    if (QMessageBox::Yes==QMessageBox::question(this, tr("Clear Cache"), tr("Clear all cached album covers?"))) {
        Core::Images::self()->clearDiskCache();
    }
}

void Ui::PreferencesDialog::buttonClicked(QAbstractButton *btn) {
    switch (box->standardButton(btn)) {
    case QDialogButtonBox::Apply:
        save();
        break;
    case QDialogButtonBox::Ok:
        save();
        accept();
        break;
    case QDialogButtonBox::Cancel:
    case QDialogButtonBox::Close:
        reject();
        break;
    }
}

void Ui::PreferencesDialog::save() {
    #ifdef QT_QTDBUS_FOUND
    Dbus::Notify::self()->setEnabled(showNotifications->isChecked());
    #elif defined Q_OS_MAC
    Mac::Notify::self()->setEnabled(showNotifications->isChecked());
    #endif
    rendererView->setScrollQueue(autoScrollPlayQueue->isChecked());
}
