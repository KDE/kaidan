<!--
SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>

SPDX-License-Identifier: CC0-1.0
-->

# Contributing

[TOC]

## Introduction

This is a guideline for contributing to Kaidan.
There is also a guide for a [basic setup](https://invent.kde.org/network/kaidan/-/wikis/setup) targeted at contributors which are unfamiliar with KDE Identity, GitLab or Git and want to start contributing quickly.

## Branches

In order to contribute to Kaidan, please create branches on your forked repository and submit merge requests for them.
Please do not create branches on Kaidan's main repository or push your commits directly to its *master* branch.

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
1. Mark the MR as draft as long as you are working on the MR and mark it as ready as soon as it should be reviewed.
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

If you add or update a functionality specified by an [XMPP Extension Protocol (XEP)](https://xmpp.org/extensions/), adjust the [Description of a Project (DOAP) file](misc/doap.xml) accordingly.

## Configuration and Database

Kaidan uses a configuration file to store settings such as the last window size.
On Linux, that configuration file is located at `.config/kaidan/kaidan.conf`.

Kaidan's database is an [SQLite](https://sqlite.org) file.
It stores, for example, contacts and messages.
On Linux, you can find it at `.local/kaidan/kaidan.sqlite3`.
To open it, you need an SQLite application (e.g., `sqlitebrowser`, use `sudo apt install sqlitebrowser` to install it on Debian-based systems)

### Configuration Options

There are some configuration options that cannot be set via the user interface and that are mostly useful for testing.

| Name | Type | Description |
|--|--|--|
| `auth/tlsErrorsIgnored` | `bool` | If enabled, invalid certificates are also accepted. |
| `auth/tlsRequirement` | `int` | `0`: Use TLS if available, `1`: Disable TLS, `2`: Require TLS |

## Testing

The environment variable `KAIDAN_PROFILE` can be set to run Kaidan with custom configuration and database files.
It defines their filename suffixes after a separating `-` while their file extensions cannot be changed.
In combination with the command-line option `--multiple`, multiple instances of Kaidan can be run simultaneously with different profiles.
E.g., if you set `KAIDAN_PROFILE=test`, the configuration file will be `kaidan-test.conf` and the database file `messages-test.sqlite3`.

## Logging

The environment variable [`QT_LOGGING_RULES`](https://doc.qt.io/qt-6/qloggingcategory.html#logging-rules) controls what is logged.
In order to print all log messages, you can set `QT_LOGGING_RULES=im.kaidan.*=true`.
You can specify a category and type (i.e., log level) to print only specific log messages via `QT_LOGGING_RULES=im.kaidan.<category>.<type>=true` (replace `<category>` and `<type>` with the desired values).

Log messages containing any of the strings defined in the environment variable `KAIDAN_LOG_FILTER` are filtered out of the log.
The strings need to be separated by semicolons (e.g., `KAIDAN_LOG_FILTER=a log message part;another log message part`).

[KDebugSettings](https://apps.kde.org/de/kdebugsettings/) can be used as a graphical alternative.
Once you inserted `kaidan/build/kaidan.categories`, you can manage how to log for Kaidan there.

Kaidan prints QXmpp's log messages as well.
You can disable that by setting `QT_LOGGING_RULES=im.kaidan.xmpp.*=false`.
[LogHandler](src/LogHandler.cpp) specifies the QXmpp-specific logging type.
QXmpp mainly logs the XML data exchanged between Kaidan and the XMPP servers it is connected to.

## Styles

A style influences Kaidan's look.
It can be set via the environment variable `QT_QUICK_CONTROLS_STYLE`.
Kaidan's default style is `org.kde.desktop`.
You can run Kaidan using Android's `Material` style with `QT_QUICK_CONTROLS_STYLE=Material kaidan`.

## Mobile Devices

Kaidan can be run on desktop devices as well as on mobile devices with touchscreens.
A user interface optimized for mobile devices can be applied via the environment variable `QT_QUICK_CONTROLS_MOBILE`.
You can run Kaidan with `QT_QUICK_CONTROLS_MOBILE=true kaidan` in order to get its mobile view.

## Coding Style

Kaidan complies with [KDE's coding style](https://community.kde.org/Policies/Frameworks_Coding_Style).
All code changes are automatically formatted by [KDE's Git pre-commit hook](https://community.kde.org/Policies/Frameworks_Coding_Style#Clang-format_automatic_code_formatting).

## C++

C++ files (except unit tests) are stored in Kaidan's [`src` directory](src).
If you add such a new file, please add it to the related [CMakeLists file](src/CMakeLists.txt).

## QML

Kaidan uses [QML](https://doc.qt.io/qt-6/qmlapplications.html) for its user interface.
New QML files must be added to the related [Qt resource collection file](src/qml/qml.qrc).
You can add your file to the end of the listings without applying any order.
Please do not change the order of existing files.

## JavaScript

Please use `let` or `const` instead of `var` when you define a variable in JavaScript within a QML file.

## Kirigami

Kaidan depends on the user interface framework [Kirigami](https://api.kde.org/frameworks/kirigami/html/index.html).
Please use its visual components within Kaidan instead of creating own ones as far as it makes sense.
You can have a look at the components Kirigami provides by opening the [Kirigami Gallery](https://invent.kde.org/sdk/kirigami-gallery) (use `sudo apt install kirigami-gallery` to install it and `kirigami2gallery` to run it on Debian-based systems).

## Translations

Kaidan is translated via [KDE Localization](https://l10n.kde.org/stats/gui/trunk-kf5/package/kaidan/).
In order to make translations possible, you need to use `qsTr("<text>")` (Example: `qsTr("Login")`) for QML `string`'s and `tr(<text>)` (Example: `tr("Online")`) for `QString`s in C++.

## Icons

For using an icon as a user interface element such as [`Kirigami.Icon`](https://api.kde.org/frameworks/kirigami/html/classIcon.html) or [`QtQuick.Controls.Button`](https://doc.qt.io/qt-5/qml-qtquick-controls2-button.html), you need to set the actual icon as its [`source`](https://api.kde.org/frameworks/kirigami/html/classIcon.html#ab04bfe8d23fdd9779421aadaaaa022f4) resp. [`name`](https://doc.qt.io/qt-5/qml-qtquick-controls2-abstractbutton.html#icon.name-prop) property.
All icons used by Kaidan must be referenced in the related [Qt resource collection file](kirigami-icons.qrc).
Kaidan's default icon theme is [Breeze](https://invent.kde.org/frameworks/breeze-icons).

Instead of using new icons, search and use icons that are already used for similar purposes.
If your purpose needs a new icon that is not yet used by Kaidan, you can find one with [Icon Explorer](https://invent.kde.org/plasma/plasma-sdk/-/tree/master/iconexplorer) (use `sudo apt install plasma-sdk` to install it and `iconexplorer` to run it on Debian-based systems).
Via corresponding buttons, you can check whether an icon is available for Breeze and if so, retrieve the icon's name to be used in Kaidan's code and the icon's path to be used in the related [Qt resource collection file](kirigami-icons.qrc).

Always make sure that you use the right path within [Breeze's icon directory](https://invent.kde.org/frameworks/breeze-icons/-/tree/master/icons).
Cuttlefish will open the chosen icon of your system's theme.
If that is not Breeze, you need to find the corresponding icon in Breeze's icon directory and use that path.

The system Kaidan is run on can apply a different icon theme than Kaidan's default.
For good compatibility with various icon themes, it is often better to use the `-symbolic.svg` variant of an icon.

## Graphics

The preferred format for graphics in Kaidan is *SVG*.
If SVG is not applicable like for screenshots, the graphic should have the format *PNG*.
New graphics that are used at runtime must be added to the related [Qt resource collection file](data/images/images.qrc).

### Changes

If you change a graphic, it might be necessary to run [graphic rendering tool](utils/render-graphics.sh) in order to generate new PNG files from their original SVG files.
Once the PNG files are created, the graphics need to be updated everywhere they are used (e.g., on Mastodon or in Kaidan's group chat).

### Optimization

In any case, the new or modified graphic must be [optimized](https://invent.kde.org/network/kaidan/-/wikis/optimizing-graphics) before adding it to a commit.

### Copyright

Kaidan complies with [REUSE](https://reuse.software).
Each file has a [copyright notice](https://reuse.software/faq/#standard-copyright).
Such a notice contains the first year a contributor edited the corresponding file.

Once a contributor created a new file or made a significant change to an existing one, a copyright notice has to be added.
No additional copyright notice should be added to existing files with a [CC0](https://spdx.org/licenses/CC0-1.0.html) license.

### Logo

If the logo is changed, it has to be done in a separate commit.
Furthermore, the logo has to be updated on multiple other places:

1. For this GitLab project by creating a [sysadmin request](https://go.kde.org/systickets) and providing a link to a PNG version in its description.
1. In the [repository of Kaidan's website](https://invent.kde.org/websites/kaidan-im) by updating all instances of `favicon*` and `logo*`.
1. On [Kaidan's Mastodon profile](https://fosstodon.org/@kaidan) by uploading a new avatar.
1. In [Kaidan's group chat](xmpp:kaidan@muc.kaidan.im?join) by uploading a new avatar.

## Other Files

Other files that are used at runtime must be added to the related [Qt resource collection file](data/data.qrc).
The [XMPP provider list](data/providers.json) from [XMPP Providers](https://invent.kde.org/melvo/xmpp-providers) is an example for such a file.

## Notifications

[Notifications](src/Notifications.cpp) are triggered via [KNotifications](https://api.kde.org/frameworks/knotifications/html/index.html).
The [configuration file](misc/kaidan.notifyrc) is used by KNotifications.
It is automatically installed when you install Kaidan.
Remember to install Kaidan again if you modified that file in order to see any changes.

## Unit Tests

Unit tests are stored in Kaidan's [`tests` directory](tests).
If you add a new C++ file to that directory, please add it to the related [CMakeLists file](tests/CMakeLists.txt).
In order to build the unit tests, you need to enable the CMake build option `BUILD_TESTING` (i.e., add `-DBUILD_TESTING=ON` to the `cmake` command for building Kaidan).

## Builds and Dependencies

Dependencies are mainly managed by Kaidan's [root CMakeLists file](CMakeLists.txt).
When you add, update or remove dependencies, update the [README](README.md) and the [building guides](https://invent.kde.org/network/kaidan/-/wikis/home#building-kaidan-from-sources) as well.

### Packages

Kaidan is [packaged for several operating systems](https://repology.org/project/kaidan/versions).

### Test Builds

The GitLab templates in Kaidan's [GitLab CI/CD configuration](.gitlab-ci.yml) automatically run jobs to check whether Kaidan can be built on the following operating systems:

* [Android](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/android-qt6.yml): [Dependencies](https://invent.kde.org/sysadmin/ci-images/-/blob/master/android-qt610-ci/CI-Craft-Packages.shelf)
* [FreeBSD](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/freebsd-qt6.yml): [Dependencies](https://invent.kde.org/sysadmin/ci-images/-/blob/master/freebsd150-qt610/ports-list)
* [Linux with latest stable Qt version](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/linux-qt6.yml): [Dependencies](https://invent.kde.org/sysadmin/ci-images/-/blob/master/suse-qt610/build.sh)
* [Linux with upcoming Qt version](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/linux-qt6-next.yml): [Dependencies](https://invent.kde.org/sysadmin/ci-images/-/blob/master/suse-qt611/build.sh)
* [Windows](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/windows-qt6.yml): [Dependencies](https://invent.kde.org/sysadmin/ci-images/-/blob/master/windows-msvc2022-qt610/CI-Craft-Packages.shelf)

Software on KDE Invent that runs one of the mentioned GitLab jobs for its own default branch is available as a dependency via the same job for Kaidan too.
In all other cases, the software needs to be added as a dependency in the referenced files.

### Flatpak

To get the most up-to-date state of Kaidan or if the operating system does not provide a regular package, Kaidan is available for Flatpak as well.
There is a [local Flatpak configuration](.flatpak-manifest.json) (called [*manifest*](https://docs.flatpak.org/en/latest/manifests.html)).
It can be used to build Kaidan based on the repository's current state.
Dependencies that are not configured by [KDE's Flatpak runtime](https://invent.kde.org/packaging/flatpak-kde-runtime) via its file `org.kde.Sdk.json.in`, need to be added to Kaidan's manifest.

#### Development Flatpak Builds

The repository's current state is automatically built by including a [GitLab Flatpak template](https://invent.kde.org/sysadmin/ci-utilities/-/blob/master/gitlab-templates/flatpak.yml) in Kaidan's [GitLab CI/CD configuration](.gitlab-ci.yml).
The resulting `flatpak` job uses the local manifest and is run on any branch that changes it.

You can download Flatpak builds for a specific branch via `https://invent.kde.org/<user>/kaidan/-/jobs/artifacts/<branch>/raw/kaidan.flatpak?job=flatpak` (replace `<user>` and `<branch>` with the desired values).
Alternatively, you can navigate to the Flatpak build via the corresponding CI job page and its *Job artifacts* section.
Once downloaded, you can install it for the current user via `flatpak install --user <download directory>/kaidan.flatpak` (replace `<download directory>` with the desired value).
After installing Kaidan, it can be run via `flatpak run --user im.kaidan.kaidan`.
Environment variables can be passed via `--env="<variable>=<value>"` (replace `<variable>` and `<value>` with the desired values).
To remove Kaidan, execute `flatpak remove --user im.kaidan.kaidan`.

In addition, the `flatpak` job is run on each change of the [`master` branch](https://invent.kde.org/network/kaidan/-/tree/master).
That [latest development state](https://invent.kde.org/network/kaidan/-/wikis/using/flatpak) is available via Kaidan's Nightly Flatpak repository.

#### Stable Flatpak Builds

There is also a [Flatpak configuration](https://github.com/flathub/im.kaidan.kaidan/blob/master/im.kaidan.kaidan.json) for [stable builds on Flathub](https://flathub.org/apps/details/im.kaidan.kaidan).
See [KDE's Flatpak documentation](https://develop.kde.org/docs/packaging/flatpak/publishing/) for more information.

### KDE Craft

There is a [KDE Craft configuration](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/kde/unreleased/kaidan/kaidan.py) (called [*blueprint*](https://community.kde.org/Craft/Blueprints)).
That is used to automatically build the [`master` branch](https://invent.kde.org/network/kaidan/-/tree/master) for the following systems:

* [Windows](https://invent.kde.org/network/kaidan/-/jobs/artifacts/master/browse/kde-ci-packages/?job=craft_windows_qt6_x86_64)
* [macOS ARM](https://invent.kde.org/network/kaidan/-/jobs/artifacts/master/browse/kde-ci-packages/?job=craft_macos_qt6_arm64)
* [macOS x86](https://invent.kde.org/network/kaidan/-/jobs/artifacts/master/browse/kde-ci-packages/?job=craft_macos_qt6_x86_64)

It is sometimes needed to update the blueprints for [QXmpp](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/qt-libs/qxmpp/qxmpp.py) and [libomemo-c](https://invent.kde.org/packaging/craft-blueprints-kde/-/blob/master/libs/libomemo-c/libomemo-c.py).
If there is no blueprint for a dependency in the [KDE Craft Blueprints repository](https://invent.kde.org/packaging/craft-blueprints-kde), a new blueprint needs to added before it can be added as a dependency to Kaidan's blueprint.
That way, Kaidan can be built correctly by KDE's automated process.
As soon as the configuration files are updated and Kaidan is automatically built, the corresponding files can be downloaded.

## Releases

Kaidan's releases are marked by [tags](https://invent.kde.org/network/kaidan/-/tags).
For each release, its source code and the source code's signature is [uploaded](https://download.kde.org/unstable/kaidan/) by one of Kaidan's maintainers.

The [release tool](utils/release.sh) releases a new version of Kaidan.
Before running the tool, make sure that your public OpenPGP key is added to the [release keyring](https://invent.kde.org/sysadmin/release-keyring).
It allows others (especially KDE's administrators) to verify your signature.

### Versioning

Kaidan uses [Semantic Versioning](https://semver.org).
There are three different kinds of releases related to the parts in the version number:

* **X**: If Kaidan introduces incompatible changes, you need to create a new **major release**.
* **Y**: If Kaidan introduces new functionality or refactoring, you need to create a new **minor release**.
* **Z**: If the latest release has some bugs, you can fix them, add the corresponding commits to the branch `Kaidan/X.Y` and create a **patch release** including those fixes on top of the latest minor release.

## Repository Information

Kaidan's [repository information](https://invent.kde.org/sysadmin/repo-metadata/-/tree/master/projects-invent/network/kaidan) must be kept up-to-date.
E.g., the branches for translations/releases and the repository's description are configured there.
