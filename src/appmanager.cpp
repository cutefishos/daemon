/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     Kate Leet <kate@cutefishos.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "appmanager.h"
#include "appmanageradaptor.h"

#include <QDBusInterface>
#include <QDebug>

AppManager::AppManager(QObject *parent)
    : QObject(parent)
    , m_backend(new QApt::Backend(this))
    , m_trans(nullptr)
{
    m_backend->init();

    new AppManagerAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QStringLiteral("/AppManager"), this);
}

void AppManager::uninstall(const QString &content)
{
    QApt::Package *package = m_backend->packageForFile(content);

    if (package) {
        m_trans = m_backend->removePackages({package});
        connect(m_trans, &QApt::Transaction::statusChanged, this, [=] (QApt::TransactionStatus status) {
            if (status == QApt::TransactionStatus::FinishedStatus) {
                notifyUninstallSuccess();
            }
        });
        m_trans->run();
    } else {
        notifyUninstallFailure();
    }
}

void AppManager::notifyUninstallFailure()
{
    QDBusInterface iface("org.freedesktop.Notifications",
                         "/org/freedesktop/Notifications",
                         "org.freedesktop.Notifications",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QList<QVariant> args;
        args << "cutefish-daemon";
        args << ((unsigned int) 0);
        args << "dialog-error";
        args << "";
        args << tr("Uninstallation failure");
        args << QStringList();
        args << QVariantMap();
        args << (int) 10;
        iface.asyncCallWithArgumentList("Notify", args);
    }
}

void AppManager::notifyUninstallSuccess()
{
    QDBusInterface iface("org.freedesktop.Notifications",
                         "/org/freedesktop/Notifications",
                         "org.freedesktop.Notifications",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QList<QVariant> args;
        args << "cutefish-daemon";
        args << ((unsigned int) 0);
        args << "process-completed-symbolic";
        args << "";
        args << tr("Uninstallation successful");
        args << QStringList();
        args << QVariantMap();
        args << (int) 10;
        iface.asyncCallWithArgumentList("Notify", args);
    }
}
