/*
 * Copyright (C) 2018 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "KisResourceLocator.h"

#include <QDebug>
#include <QList>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>

#include <kconfig.h>
#include <kconfiggroup.h>
#include <ksharedconfig.h>
#include <klocalizedstring.h>

#include "KoResourcePaths.h"
#include "KisResourceStorage.h"

const QString KisResourceLocator::resourceLocationKey {"ResourceDirectory"};
const QStringList KisResourceLocator::resourceTypeFolders = QStringList()
        << "tags"
        << "asl"
        << "bundles"
        << "brushes"
        << "gradients"
        << "paintoppresets"
        << "palettes"
        << "patterns"
        << "taskset"
        << "workspaces"
        << "symbols";

class KisResourceLocator::Private {
public:
    QString resourceLocation;
    QList<KisResourceStorageSP> storages;
    QStringList errorMessages;
};

KisResourceLocator::KisResourceLocator()
    : d(new Private())
{
}

KisResourceLocator::~KisResourceLocator()
{
}

KisResourceLocator::LocatorError KisResourceLocator::initialize(const QString &installationResourcesLocation)
{
    KConfigGroup cfg(KSharedConfig::openConfig(), "");
    d->resourceLocation = cfg.readEntry(resourceLocationKey, QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));

    QFileInfo fi(d->resourceLocation);

    if (!fi.exists()) {
        if (!QDir().mkpath(d->resourceLocation)) {
            d->errorMessages << i18n("1. Could not create the resource location at %1.", d->resourceLocation);
            return LocatorError::CannotCreateLocation;
        }
    }

    if (!fi.isWritable()) {
        d->errorMessages << i18n("2. The resource location at %1 is not writable.", d->resourceLocation);
        return LocatorError::LocationReadOnly;
    }

    if (QDir(d->resourceLocation).isEmpty()) {
        KisResourceLocator::LocatorError res = firstTimeInstallation(installationResourcesLocation);
        if (res != LocatorError::Ok) {
            return res;
        }
    }

    return LocatorError::Ok;
}

QStringList KisResourceLocator::errorMessages() const
{
    return d->errorMessages;
}

KisResourceLocator::LocatorError KisResourceLocator::firstTimeInstallation(const QString &installationResourcesLocation)
{
    Q_FOREACH(const QString &folder, resourceTypeFolders) {
        QDir dir(d->resourceLocation + '/' + folder + '/');
        if (!dir.exists()) {
            if (!dir.mkpath(dir.path())) {
                d->errorMessages << i18n("3. Could not create the resource location at %1.", dir.path());
                return LocatorError::CannotCreateLocation;
            }
        }
    }

    Q_FOREACH(const QString &folder, resourceTypeFolders) {
        QDir dir(installationResourcesLocation + "/share/krita/" + folder + '/');
        if (dir.exists()) {
            Q_FOREACH(const QString &entry, dir.entryList(QDir::Files | QDir::Readable | QDir::NoDotAndDotDot)) {
                QFile f(dir.canonicalPath() + '/'+ entry);
                bool r = f.copy(d->resourceLocation + '/' + folder + '/' + entry);
                if (!r) {
                    d->errorMessages << "Could not copy resource" << f.fileName() << "to the resource folder";
                }
            }
        }
    }

    return LocatorError::Ok;
}
