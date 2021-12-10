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
    QString packageName;

    if (package) {
        packageName = package->name();

        for (const QString &item : package->requiredByList()) {
            QApt::Package *p = m_backend->package(item);

            if (!p || !p->isInstalled())
                continue;

            if (p->recommendsList().contains(packageName))
                continue;

            if (p->suggestsList().contains(packageName))
                continue;

            if (m_backend->package(item)) {
                m_backend->package(item)->setPurge();
            }
        }

        m_trans = m_backend->removePackages({package});
        m_backend->commitChanges();

        connect(m_trans, &QApt::Transaction::statusChanged, this, [=] (QApt::TransactionStatus status) {

            if (status == QApt::TransactionStatus::FinishedStatus) {
                notifyUninstallSuccess(packageName);
            } else if (status == QApt::TransactionStatus::WaitingStatus) {
                notifyUninstalling(packageName);
            }
        });

        m_trans->run();
    } else {
        notifyUninstallFailure(packageName);
    }
}

void AppManager::notifyUninstalling(const QString &packageName)
{
    QDBusInterface iface("org.freedesktop.Notifications",
                         "/org/freedesktop/Notifications",
                         "org.freedesktop.Notifications",
                         QDBusConnection::sessionBus());
    if (iface.isValid()) {
        QList<QVariant> args;
        args << "cutefish-daemon";
        args << ((unsigned int) 0);
        args << "cutefish-installer";
        args << packageName;
        args << tr("Uninstalling");
        args << QStringList();
        args << QVariantMap();
        args << (int) 10;
        iface.asyncCallWithArgumentList("Notify", args);
    }
}

void AppManager::notifyUninstallFailure(const QString &packageName)
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
        args << packageName;
        args << tr("Uninstallation failure");
        args << QStringList();
        args << QVariantMap();
        args << (int) 10;
        iface.asyncCallWithArgumentList("Notify", args);
    }
}

void AppManager::notifyUninstallSuccess(const QString &packageName)
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
        args << packageName;
        args << tr("Uninstallation successful");
        args << QStringList();
        args << QVariantMap();
        args << (int) 10;
        iface.asyncCallWithArgumentList("Notify", args);
    }
}
