<!--
SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>

SPDX-License-Identifier: CC0-1.0
-->

# Contributing

This is a guideline for contributing to Kaidan.
There is also a guide for a [basic setup](https://invent.kde.org/network/kaidan/-/wikis/setup) targeted at contributors which are unfamiliar with KDE Identity, GitLab or Git and want to start contributing quickly.

In order to contribute to Kaidan, please create branches on your forked repository and submit merge requests for them.
Please do not create branches on Kaidan's main repository or push your commits directly to its *master* branch.

## Branches

Use the following prefixes for branch names:
* `feature/` for new features (changes with new behavior)
* `refactor/` for changes of the code structure without changing the general behavior
* `fix/` for fixes (changes for intended / correct behavior)
* `design/` for design changes
* `doc/` for documentation

## Merge Requests (MR)

Current Maintainers:
 * Linus Jahn (@lnj)
 * Jonah Brüchert (@jbbgameich)
 * Melvin Keskin (@melvo)

They are responsible for accepting MRs.
Nevertheless, everybody is welcome to review MRs and give feedback on them.

Please stick to the following steps for opening, reviewing and accepting MRs.

### For Authors

1. Create a new branch from the *master* branch to work on it.
1. Write short commit messages starting with an upper case letter and the imperative.
1. Split your commits logically.
1. Do not mix unrelated changes in the same MR.
1. Create an MR with the *master* branch as its target.
1. Add `Draft: ` in front of the MR's title as long as you are working on the MR and remove it as soon as it is ready to be reviewed.
1. A maintainer and possibly other reviewers will give you feedback.
1. Improve your MR according to their feedback, push your commits and close open threads via the *Resolve thread* button.
1. If necessary, modify, reorder or squash your commits and force-push (`git push -f`) the result to the MR's branch.
1. As soon as all threads on your MR are resolved, a maintainer will merge your commits into the *master* branch.

Please do not merge your commits into the *master* branch by yourself.
If maintainers approved your MR but have not yet merged it, that probably means that they are waiting for the approval of additional maintainers.
Feel free to ask if anything is unclear.

### For Reviewers

1. Provide detailed descriptions of found issues to the author.
1. Try to give the author concrete proposals for improving the code via the *Insert suggestion* button while commenting.
1. If the proposals are too complicated, create and push a commit with your proposal to your own fork of Kaidan and open an MR with the author's MR branch as its target.
1. In case you are a maintainer:
	1. If you think that no additional review is needed, make editorial modifications (such as squashing the commits) and merge the result directly.
	1. If you would like to get (more) feedback from other maintainers, approve the MR using the *Approve* button and mention other maintainers to review the MR.
1. In case you are not a maintainer and you think that the MR is ready to be merged, approve the MR using the *Approve* button.

Reviews have to be done by at least one maintainer not involved as the MR's author or co-author.

## Features

If you add or update a functionality specified by an [XMPP Extension Protocol (XEP)](https://xmpp.org/extensions/), adjust the [Description of a Project (DOAP) file](/misc/doap.xml) accordingly.

## Configuration and Database Files

Kaidan uses a configuration file to store settings such as the last window size.
On Linux, that configuration file is located at `.config/kaidan/kaidan.conf`.

Kaidan's database is an SQLite file.
It stores, for example, contacts and messages.
On Linux, you can find it at `.local/kaidan/kaidan.sqlite3`.
To open it, you need an SQLite application (e.g., `sqlitebrowser`, use `sudo apt install sqlitebrowser` to install it on Debian-based systems)

## Testing

The environment variable `KAIDAN_PROFILE` can be set to run Kaidan with custom configuration and database files.
It defines their filename suffixes after a separating `-` while their file extensions cannot be changed.
In combination with the command-line option `--multiple`, multiple instances of Kaidan can be run simultaneously with different profiles.
E.g., if you set `KAIDAN_PROFILE=test`, the configuration file will be `kaidan-test.conf` and the database file `messages-test.sqlite3`.

## Styles

A style influences Kaidan's look.
It can be set via the environment variable `QT_QUICK_CONTROLS_STYLE`.
Kaidan's default style is `org.kde.desktop`.
You can run Kaidan using Android's `Material` style with `QT_QUICK_CONTROLS_STYLE=Material kaidan`.

## Mobile Devices

Kaidan can be run on desktop devices as well as on mobile devices with touchscreens.
A user interface optimized for mobile devices can be applied via the environment variable `QT_QUICK_CONTROLS_MOBILE`.
You can run Kaidan with `QT_QUICK_CONTROLS_MOBILE=true kaidan` in order to get its mobile view.

## Files

If you add a new C++ file, please add it to `src/CMakeLists.txt`.
QML files must be added to `src/qml/qml.qrc`.
You can add your file to the end of the listings without applying any order.
Please do not change the order of existing files.

## Notifications

Notifications are triggered by `src/Notifications` via [KNotifications](https://api.kde.org/frameworks/knotifications/html/index.html).
The configuration file `misc/kaidan.notifyrc` is used by KNotifications.
It is automatically installed when you install Kaidan.
Remember to install Kaidan again if you modified that file in order to see any changes.

## User Interface

Kaidan depends on the user interface framework [Kirigami](https://api.kde.org/frameworks/kirigami/html/index.html).
Please use its visual components within Kaidan instead of creating own ones as far as it makes sense.
You can have a look at the components Kirigami provides by opening the [Kirigami Gallery](https://invent.kde.org/sdk/kirigami-gallery) (use `sudo apt install kirigami-gallery` to install it and `kirigami2gallery` to run it on Debian-based systems).

## Icons

For using an icon as a user interface element such as [`Kirigami.Icon`](https://api.kde.org/frameworks/kirigami/html/classIcon.html) or [`QtQuick.Controls.Button`](https://doc.qt.io/qt-5/qml-qtquick-controls2-button.html), you need to set the actual icon as its [`source`](https://api.kde.org/frameworks/kirigami/html/classIcon.html#ab04bfe8d23fdd9779421aadaaaa022f4) resp. [`name`](https://doc.qt.io/qt-5/qml-qtquick-controls2-abstractbutton.html#icon.name-prop) property.
All icons used by Kaidan must be referenced in [`kirigami-icons.qrc`](https://invent.kde.org/melvo/kaidan/-/blob/master/kirigami-icons.qrc).
Kaidan's default icon theme is [Breeze](https://invent.kde.org/frameworks/breeze-icons).

Instead of using new icons, search and use icons that are already used for similar purposes.
If your purpose needs a new icon that is not yet used by Kaidan, you can find one with [Cuttlefish](https://invent.kde.org/plasma/plasma-sdk/-/tree/master/cuttlefish) (use `sudo apt install plasma-sdk` to install it and `cuttlefish` to run it on Debian-based systems).
Via corresponding buttons, you can check whether an icon is available for Breeze and if so, retrieve the icon's name to be used in Kaidan's code and the icon's path to be used in `kirigami-icons.qrc`.

Always make sure that you use the right path within [Breeze's icon directory](https://invent.kde.org/frameworks/breeze-icons/-/tree/master/icons).
Cuttlefish will open the chosen icon of your system's theme.
If that is not Breeze, you need to find the corresponding icon in Breeze's icon directory and use that path.

The system Kaidan is run on can apply a different icon theme than Kaidan's default.
For good compatibility with various icon themes, it is often better to use the `-symbolic.svg` variant of an icon.

## Builds and Dependencies

On a daily basis, Kaidan is automatically built for various systems.
Those *nightly builds* are based on Kaidan's *master* branch.

Kaidan is packaged for several [Linux distributions](https://repology.org/project/kaidan/versions).
For distributions supporting Flatpak, there is a [Flatpak configuration](https://invent.kde.org/network/kaidan/-/blob/master/.flatpak-manifest.json) (called [*manifest*](https://docs.flatpak.org/en/latest/manifests.html)) and its [reference](https://invent.kde.org/packaging/flatpak-kde-applications/-/blob/master/im.kaidan.kaidan.remoteapp) for [nightly builds](https://invent.kde.org/network/kaidan/-/wikis/using/flatpak).
There is also a [Flatpak configuration](https://github.com/flathub/im.kaidan.kaidan/blob/master/im.kaidan.kaidan.json) for [stable builds on Flathub](https://flathub.org/apps/details/im.kaidan.kaidan).
In addition, there is a [KDE Craft configuration](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/kde/unreleased/kaidan/kaidan.py) (called [*blueprint*](https://community.kde.org/Craft/Blueprints)) for [Windows, macOS and Android builds](https://binary-factory.kde.org/search/?q=kaidan).

Dependencies are mainly managed by Kaidan's [CMakeLists](/CMakeLists.txt) file.
When you add or remove dependencies, update the [README](/README.md#dependencies) and the [building guides](https://invent.kde.org/network/kaidan/-/wikis/home#building-kaidan-from-sources) as well.

You also need to modify the KDE Craft and Flatpak configuration files for Kaidan.
Only dependencies that are not configured by [KDE's Flatpak runtime](https://invent.kde.org/packaging/flatpak-kde-runtime) via its file `org.kde.Sdk.json.in`, need to be added to Kaidan's Flatpak configuration.
It is sometimes needed to update the KDE Craft configurations for [QXmpp](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/qt-libs/qxmpp/qxmpp.py), [libomemo-c](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/libs/libomemo-c/libomemo-c.py) and [zxing-cpp](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/libs/zxing-cpp/zxing-cpp.py) as well.
That way, Kaidan can be built correctly by KDE's automated process.
As soon as the configuration files are updated and Kaidan is automatically built, the corresponding files can be downloaded.

## Releases

Kaidan's releases are marked by [tags](https://invent.kde.org/network/kaidan/-/tags).
For each release, its source code and the source code's signature is [uploaded](https://download.kde.org/unstable/kaidan/) by one of Kaidan's maintainers.

## Translations

Kaidan is translated via [KDE Localization](https://l10n.kde.org/stats/gui/trunk-kf5/package/kaidan/).
In order to make translations possible, you need to use `qsTr("<text>")` (Example: `qsTr("Login")`) for QML `string`'s and `tr(<text>)` (Example: `tr("Online")`) for `QString`s in C++.

## QML and Javascript

Please use `let` or `const` instead of `var` when you define a variable in JavaScript within a QML file.

## Graphics

The preferred format for graphics in Kaidan is *SVG*.
If SVG is not applicable like for screenshots, the graphic should have the format *PNG*.

### Changes

If you change a graphic, it might be necessary to run `utils/render-graphics.sh` in order to generate new PNG files from their original SVG files.
Once the PNG files are created, the graphics need to be updated everywhere they are used (e.g., on Mastodon or in Kaidan's group chat).

### Optimization

In any case, the new or modified graphic must be [optimized](https://invent.kde.org/network/kaidan/-/wikis/optimizing-graphics) before adding it to a commit.

### Copyright

The *LICENSE* file must be updated if there are copyright changes:

1. Add to *utils/generate-license.py* a new `CopyrightTarget` for a new graphic or change an existing one for a modification of an existing graphic.
1. Execute `utils/generate-license.py > LICENSE` for updating the *LICENSE* file.
1. Add those two file changes to the same commit which contains the new or modified graphic.

### Logo

If the logo is changed, it has to be done in a separate commit.
Furthermore, the logo has to be updated on multiple other places:

1. For this GitLab project by creating a [sysadmin request](https://go.kde.org/systickets) and providing a link to a PNG version in its description.
1. In the [repository of Kaidan's website](https://invent.kde.org/websites/kaidan-im) by updating all instances of `favicon*` and `logo*`.
1. On [Kaidan's Mastodon profile](https://fosstodon.org/@kaidan) by uploading a new avatar.
1. In [Kaidan's group chat](xmpp:kaidan@muc.kaidan.im?join) by uploading a new avatar.
