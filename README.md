<!--
SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>

SPDX-License-Identifier: CC0-1.0
-->

# Kaidan - User-friendly and modern chat app for every device

[![Kaidan MUC](https://search.jabbercat.org/api/1.0/badge?address=kaidan@muc.kaidan.im)](https://i.kaidan.im)
[![license](https://img.shields.io/badge/License-GPLv3%2B%20%2F%20CC%20BY--SA%204.0-blue.svg)](https://raw.githubusercontent.com/kaidanim/kaidan/master/LICENSE)
[![Donations](https://img.shields.io/liberapay/patrons/kaidan.svg?logo=liberapay)](https://liberapay.com/kaidan)

![Kaidan screenshot](https://www.kaidan.im/images/screenshot.png)

<a href="https://repology.org/project/kaidan/versions">
    <img src="https://repology.org/badge/vertical-allrepos/kaidan.svg" alt="Packaging status" align="right">
</a>

## About

[Kaidan][kaidan-website] is a simple, user-friendly and modern chat client. It
uses the open communication protocol [XMPP (Jabber)][xmpp]. The user interface
makes use of [Kirigami][kirigami-website] and [QtQuick][qtquick], while the
back-end of Kaidan is entirely written in C++ using [Qt][qt] and the Qt-based
XMPP library [QXmpp][qxmpp].

Kaidan runs on mobile and desktop systems including Linux, Windows, macOS,
Android, Plasma Mobile and Ubuntu Touch.
Unfortunately, we are not able to provide builds for all platforms at the moment
due to little developer resources.

Kaidan does *not* have all basic features yet and has still some stability
issues. Do not expect it to be as good as the currently dominating instant
messaging clients.

If you are interested in the technical features, you can have a
look at Kaidan's [overview page][overview] including XEPs and RFCs.

## Using and Building Kaidan

Downloadable builds are available on [Kaidan's download page][downloads].
Instructions for using ready-made (nightly / stable) builds and for building
Kaidan yourself can be found in our [Wiki][wiki].

## Dependencies

Here are the general dependencies of Kaidan:
 * [Qt][qt-build-sources] (Core Concurrent Qml Quick Svg Sql QuickControls2 Xml Multimedia Positioning Location) (>= 5.15.0)
 * [QXmpp][qxmpp] (with OMEMO) (>= 1.5.0)
 * [Kirigami 2][kirigami-repo] (>= 5.67.0)
 * [Kirigami Addons][kirigami-addons] (>= 0.7.0)
 * [KQuickImageEditor][kquickimageeditor] (>= 0.2.0)
 * [ECM (extra-cmake-modules)][ecm]
 * [ZXing-cpp][zxing-cpp] (>= 1.1.1)
 * [KNotifications][knotifications] (`-DUSE_KNOTIFICATIONS=OFF` to disable)

## Contributing

If you are interested in contributing to Kaidan, please have a look at our
[contribution guidelines][contributing]. If you want to improve Kaidan's
website, feel free to visit its [project site][kaidan-website-repo].

## Security Contact

If you have found a security issue in Kaidan or related projects, you can find
information on how to proceed in our [security.txt][securitytxt] or at the
[KDE Security website][kdesecurity].

[contributing]: CONTRIBUTING.md
[downloads]: https://www.kaidan.im/download/
[ecm]: https://api.kde.org/ecm/manual/ecm.7.html
[kaidan-website]: https://kaidan.im
[kaidan-website-repo]: https://invent.kde.org/websites/kaidan-im
[kirigami-addons]: https://invent.kde.org/libraries/kirigami-addons
[kirigami-repo]: https://invent.kde.org/frameworks/kirigami
[kirigami-website]: https://kde.org/products/kirigami/
[knotifications]: https://api.kde.org/frameworks/knotifications/html/index.html
[kquickimageeditor]: https://invent.kde.org/libraries/kquickimageeditor
[qt]: https://www.qt.io/
[qt-build-sources]: https://doc.qt.io/qt-5/build-sources.html
[qtquick]: https://wiki.qt.io/Qt_Quick
[qxmpp]: https://github.com/qxmpp-project/qxmpp
[wiki]: https://invent.kde.org/network/kaidan/-/wikis/home
[overview]: https://xmpp.org/software/clients/kaidan/
[xmpp]: https://xmpp.org
[zxing-cpp]: https://github.com/nu-book/zxing-cpp
[securitytxt]: https://www.kaidan.im/.well-known/security.txt
[kdesecurity]: https://kde.org/info/security/
