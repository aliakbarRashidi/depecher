#include "DBusAdaptor.hpp"
#include "singletons/PageAppStarter.hpp"
#include "FileWorker.hpp"

#include "tdlibQt/TdlibJsonWrapper.hpp"
#include "tdlibQt/models/StickerModel.hpp"
#include "tdlibQt/models/ChatsModel.hpp"
#include "tdlibQt/models/MessagingModel.hpp"
#include "tdlibQt/models/StickerModel.hpp"
#include "tdlibQt/TelegramProfileProvider.hpp"
#include "tdlibQt/include/AuthenticationHandler.hpp"
#include "tdlibQt/items/ProxyDAO.hpp"
#include "tdlibQt/items/AboutMeDAO.hpp"

#include <QDebug>
#include <QQuickView>
#include <QQmlContext>
#include <QQmlEngine>
#include "sailfishapp.h"

static const QString c_dbusServiceName = QStringLiteral("org.blacksailer.depecher");
static const QString c_dbusObjectPath = QStringLiteral("/org/blacksailer/depecher");
static const QString c_dbusInterface = QStringLiteral("org.blacksailer.depecher");
static const QString c_dbusMethod = QStringLiteral("showApp");

DBusAdaptor::DBusAdaptor(QGuiApplication *parent) : app(parent)
{
//    connect(app, &QGuiApplication::destroyed, this, &DBusAdaptor::stopDaemon);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    new DepecherAdaptor(this);
    pagesStarter = new PageAppStarter(this);
    qDebug() << "Register object" << dbus.registerObject(c_dbusObjectPath, this); //object path
    if (!isRegistered()) {
        bool ready = dbus.registerService(c_dbusServiceName);
        qDebug() << "Register service" << ready;
        if (!ready) {
            qWarning() << "Service already registered, exiting...";
            app->quit();
        }
    }
}
DBusAdaptor::~DBusAdaptor()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();

    dbus.unregisterObject(c_dbusObjectPath); //object path
    bool ready = dbus.unregisterService(c_dbusServiceName);
    qDebug() << "Unregister service" << ready; //object path

}

bool DBusAdaptor::isRegistered()
{
    return QDBusConnection::sessionBus().interface()->isServiceRegistered(c_dbusServiceName);
}

bool DBusAdaptor::raiseApp()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    QDBusMessage showUi = QDBusMessage::createMethodCall(
                              c_dbusServiceName,
                              c_dbusObjectPath,
                              c_dbusInterface,
                              c_dbusMethod);
    QStringList args;
    args << "Start";
    showUi << QVariant::fromValue(args);

    auto result = dbus.call(showUi);
    if (result.errorName() != "")
        qDebug() << result.errorName() << result.errorMessage();
    return true;
}

void DBusAdaptor::showApp(const QStringList &cmd)
{
    if (!view) {
        qDebug() << "Construct view";
        view = SailfishApp::createView();

        connect(view, &QQuickView::destroyed, this, &DBusAdaptor::onViewDestroyed);
        connect(view, SIGNAL(closing(QQuickCloseEvent *)), this,  SLOT(onViewClosing(QQuickCloseEvent *)));

        qmlRegisterUncreatableType<tdlibQt::Enums>("tdlibQtEnums", 1, 0, "TdlibState",
                "Error class uncreatable");
        qmlRegisterType<FileWorker>("depecherUtils", 1, 0, "FileWorker");
        qmlRegisterType<tdlibQt::AboutMeDAO>("TelegramItems", 1, 0, "AboutMeDAO");
        qmlRegisterType<tdlibQt::ProxyDAO>("TelegramItems", 1, 0, "ProxyDAO");
        qmlRegisterType<tdlibQt::MessagingModel>("TelegramModels", 1, 0, "MessagingModel");
        qmlRegisterType<tdlibQt::ChatsModel>("TelegramModels", 1, 0, "ChatsModel");
        qmlRegisterType<tdlibQt::StickerModel>("TelegramModels", 1, 0, "StickerModel");

        qmlRegisterType<tdlibQt::AuthenticationHandler>("TelegramAuthentication", 1, 0,
                "TelegramAuthenticationHandler");
        view->setTitle("Depecher");
        view->rootContext()->setContextProperty("c_telegramWrapper", tdlibQt::TdlibJsonWrapper::instance());
        view->rootContext()->setContextProperty("c_PageStarter", pagesStarter);
        view->engine()->addImageProvider(QLatin1String("depecherDb"), new tdlibQt::TelegramProfileProvider);
        view->setSource(SailfishApp::pathTo("qml/app.qml"));
        view->show();
    } else if (view->windowState() == Qt::WindowNoState) {
        view->create();
        view->showFullScreen();
    } else {
        view->raise();
        view->requestActivate();
    }
}

void DBusAdaptor::openConversation(const qlonglong &chatId)
{
    pagesStarter->addPage(chatId);
    showApp(QStringList());
}

void DBusAdaptor::onViewDestroyed()
{
    view = nullptr;
}

void DBusAdaptor::onViewClosing(QQuickCloseEvent *v)
{
    Q_UNUSED(v)
    view->destroy();
    view->deleteLater();
}
