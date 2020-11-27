#include "pages/updatespage.h"
#include "backend/packagekithelper.h"
#include "backend/appstreamhelper.h"
#include "backend/settings.h"
#include <QVBoxLayout>
#include <DGuiApplicationHelper>

UpdatesPage::UpdatesPage(MainWindow *parent)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    this->setLayout(layout);
    list = new List(tr("Updates"));
    list->setEmptyText(tr("Up to date"));

    systemUpdatesItem = tr("System Updates");

    mainwindow = parent;

    connect(list, &List::currentItemChanged, this, [ = ] (QString package) {
        if (package == systemUpdatesItem) {
            systemUpdatesPopup->exec();
        } else {
            parent->openItem(package);
        }
    });

    refreshButton = new DIconButton(this);
    connect(DGuiApplicationHelper::instance(), &DGuiApplicationHelper::themeTypeChanged, this, [ = ] {
        if (DGuiApplicationHelper::instance()->themeType() == DGuiApplicationHelper::DarkType) {
            refreshButton->setIcon(QIcon::fromTheme("refresh"));
        } else {
            refreshButton->setIcon(QIcon::fromTheme("refresh_dark"));
        }
    });
    refreshButton->setIconSize(QSize(20, 20));
    refreshButton->setDisabled(true);
    list->addHeaderWidget(refreshButton);

    updateButton = new DSuggestButton(tr("Update All"));
    updateButton->setDisabled(true);
    list->addHeaderWidget(updateButton);

    PackageKitHelper::instance()->getUpdates(this);

    layout->addWidget(list);
}

void UpdatesPage::loadData(QHash<QString, int> apps)
{
    QLocale locale;
    int totalSize = 0;
    for (int size : apps) {
        totalSize += size;
    }

    list->clear();
    appUpdates.clear();
    systemUpdates.clear();

    connect(updateButton, &DSuggestButton::clicked, this, [ = ] {
        updateButton->setDisabled(true);
        updateButton->setText(tr("Updating..."));
        refreshButton->setDisabled(true);
        PackageKitHelper::instance()->update(this);
    });

    connect(refreshButton, &DIconButton::clicked, this, [ = ] {
        updateButton->setDisabled(true);
        refreshButton->setDisabled(true);
        list->clear();
        list->unload();
        PackageKitHelper::instance()->getUpdates(this);
    });

    updateButton->setText(tr("Update All (%1)").arg(locale.formattedDataSize(totalSize)));
    updateButton->setDisabled(false);
    refreshButton->setDisabled(false);

    if (apps.size() == 0) {
        updateButton->setDisabled(true);
        mainwindow->setUpdateIndicatorVisible(false);
    } else {
        mainwindow->setUpdateIndicatorVisible(true);
    }

    for (QString app : apps.keys()) {
        if (settings::instance()->nonApps() == 0) {
            if (AppStreamHelper::instance()->hasAppData(app)) {
                appUpdates << app;
            } else {
                systemUpdates << app;
            }
        } else {
            appUpdates << app;
        }
    }
    appUpdates.sort();
    systemUpdates.sort();

    for (QString app : appUpdates) {
        list->addItem(app);
    }

    if (systemUpdates.length() > 0) {
        list->addItem(systemUpdatesItem, QIcon::fromTheme("application-x-executable"));
        QString popupText;
        for (QString app : systemUpdates) {
            popupText.append("<br><b>" + app + "</b> - " + locale.formattedDataSize(apps.value(app)));
        }
        systemUpdatesPopup = new DDialog(systemUpdatesItem, popupText);
        systemUpdatesPopup->setIcon(style()->standardIcon(QStyle::SP_MessageBoxInformation));
    }

    list->load();
}

void UpdatesPage::updatePercent(QString package, uint percent)
{
    if (appUpdates.contains(package)) {
        list->editItemText(package, QString::number(percent) + "%");
    } else {
        list->editItemText(systemUpdatesItem, QString::number(percent) + "%");
    }
}