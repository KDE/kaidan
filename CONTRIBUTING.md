# Contributing

This is a guideline for contributing to Kaidan.
There is also a guide for a [basic setup](https://invent.kde.org/network/kaidan/-/wikis/setup) targeted at contributors which are unfamiliar with KDE Identity, GitLab or Git and want to start contributing quickly.

If you would like to contribute to Kaidan, please create branches on your repository clone and submit merge requests for them.
Please do not create branches on Kaidan's main repository or push your commits directly to its *master* branch.

## Branches

Use the following prefixes for branch names:
* `feature/` for new features (changes with new behavior)
* `refactor/` for changes of the code structure without changing the general behavior
* `fix/` for fixes (changes for intended / correct behavior)
* `design/` for design changes
* `doc/` for documentation

## Merge Requests (MR)

Currently, Linus Jahn (@lnj) and Jonah Brüchert (@jbbgameich) are the maintainers of Kaidan.
They are responsible for accepting MRs.
Nevertheless, everybody is welcome to review MRs and give feedback on them.

Please stick to the following steps for opening, reviewing and accepting MRs.

### For Authors

1. Create a new branch to work on it from the *master* branch.
1. Write short commit messages starting with an upper case letter and the imperative.
1. Split your commits logically.
1. Do not mix unrelated changes in the same MR.
1. Create a MR with the *master* branch as its target.
1. Add `Draft: ` in front of the MR's title as long as you are working on the MR and remove it as soon as it is ready to be reviewed.
1. A maintainer and possibly other reviewers will give you feedback.
1. Improve your MR according to their feedback, push your commits and close open threads via the *Resolve thread* button.
1. If necessary, modify, reorder or squash your commits and force-push (`git push -f`) the result to the MR's branch.
1. If there are no open threads on your MR, a maintainer will merge your commits into the *master* branch.

Please do not merge your commits into the *master* branch by yourself.
If maintainers approved your MR but have not yet merged it, that probably means that they are waiting for the approval of additional maintainers.
Feel free to ask if anything is unclear.

### For Reviewers

1. Provide detailed descriptions of found issues to the author.
1. Try to give the author concrete proposals for improving the code via the *Insert suggestion* button while commenting.
1. If the proposals are too complicated, create and push a commit with your proposal to your own fork of Kaidan and open a MR with the author's MR branch as its target.
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

For using an icon as a user interface element such as `Kirigami.Icon`, you need to set the actual icon as its `source` property.
All icons used by Kaidan must be referenced in `kirigami-icons.qrc`.
Kaidan's default icon theme is [Breeze](https://invent.kde.org/frameworks/breeze-icons).

Instead of using new icons, search and use icons that are already used for similar purposes.
If your purpose needs a new icon that is not yet used by Kaidan, add it to `kirigami-icons.qrc` by including the icon's path within Breeze's directory.

The system Kaidan is run on can apply a different icon theme than Kaidan's default.
For good compatibility with various icon themes, it is often better to use the `-symbolic.svg` variant of an icon.

## Graphics

The preferred format for graphics in Kaidan is *SVG*.
If SVG is not applicable like for screenshots, the graphic should have the format *PNG*.

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
1. In [Kaidan's support chat](xmpp:kaidan@muc.kaidan.im?join) by uploading a new avatar.
