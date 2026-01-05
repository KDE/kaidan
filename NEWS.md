<!-- markdownlint-disable MD022 MD032 MD041 -->
Version 0.14.0
--------------
Released: 2026-01-01

Features:
* Keep draft messages on top of pinned/unpinned chat list items (pehg)
* Optimize thumbnail creation (fazevedo)
* Display border around searched/referenced message instead of bar (melvo)
* Display border around message if its context menu is shown (melvo)
* Highlight message's avatar if hovered (melvo)
* Display provider chat (used for welcome messages and service announcements) as such (melvo)
* Always display same name and proper avatar initials for notes chats (melvo)
* Allow to cancel downloads/uploads (fazevedo)
* Allow to restart canceled uploads (fazevedo)
* Allow to resend failed message via context menu (melvo)
* Retrieve support addresses from server instead of provider list (XEP-0157: Contact Addresses for XMPP Services, XEP-0128: Service Discovery Extensions) (melvo)
* Allow adding contact by entering XMPP URI (including authentication of OMEMO 2 keys) (melvo)
* Allow joining group chat by entering XMPP URI (melvo)
* Add option for allowing SASL PLAIN (needed for servers using LDAP) to custom connection settings (melvo)
* Add filtering options to choose provider for registration (melvo)
* Cancel file transfers on disconnecting from server (fazevedo)
* Add support for offline media sharing queue (fazevedo)
* Restart downloads canceled on logout once logged in (melvo)
* Remove all related data on message removal (such as reactions and media) (melvo)

Bugfixes:
* Fix displaying message reaction details (melvo)
* Fix adding unneeded separator on mention after new line (melvo)
* Wrap hint for actions needing user to be connected to server (melvo)
* Fix displaying icons on Windows (pehg)
* Fix adding new line at cursor position (melvo)
* Ensure that own encryption devices are not removed if notes chat is removed (melvo)

Notes:
* Kaidan requires KIconThemes (for Windows) now
* Kaidan requires Qt6GuiPrivate (for Qt 6.10 or above) now
* Kaidan requires Qt 6.7 now
* Kaidan requires QXmpp 1.13 now

Version 0.13.0
--------------
Released: 2025-09-21

Features:
* Add support for using multiple accounts simultaneously (melvo)
* List accounts and show button to add new accounts (melvo)
* Show dialog to select account for global action such as adding a contact (melvo)
* Allow to enable/disable accounts instead of connecting/disconnecting them manually (melvo)
* Update nicknames of own accounts once connected (melvo)
* Show small account avatars next to regular avatars if multiple accounts are used (melvo)
* Hide global drawer handle on chat if window is narrow (melvo)
* Use PNG/.png instead of JPEG/.jpg for thumbnails to allow transparency (melvo)
* Use AAC/.m4a instead of MP3/.mp3 for voice messages to improve compatibility (melvo)
* Provide size of sent images to recipients allowing receiving client to scale thumbnails to size of original image (melvo)
* Provide size of generated thumbnails to recipients (melvo)
* Increase size of generated thumbnails (melvo)
* Show circle instead of bar for upload/download progress (melvo)
* Try all providers on connection error during automatic registration (melvo)
* Add message forwarding (melvo)
* Enable voice message recording via Flatpak (melvo)
* Store account passwords encrypted if password manager is available (fazevedo)
* Apply consistent criteria for all message corrections (melvo)
* Add support to mark messages locally in order to reply to them later or to quickly find important messages (melvo)
* Reuse SASL 2 user agent and FAST token on every restart for faster connection establishment (melvo)

Bugfixes:
* Fix selecting media via long press in media overview (melvo)
* Fix OMEMO initialization (melvo)
* Fix displaying geo location map (melvo)
* Fix showing hints on invalid input of various input fields (melvo)
* Fix name/date of chat list item moving if counter for unread messages dis-/appears (melvo)
* Fix counter for unread messages (melvo)
* Fix handling removed message reactions (melvo)
* Fix canceling personal data sharing via contact details (melvo)
* Fix finding existing notifications for personal data sharing requests (melvo)
* Fix cursor behavior in message input field by allowing vertical cursor movements while participant picker is closed and prohibiting horizontal cursor movements while participant picker is open (melvo)

Notes:
* Kaidan requires QtKeychain 0.15 now
* Kaidan requires QXmpp 1.11 now

Version 0.12.2
--------------
Released: 2025-03-30

Bugfixes:
* Fix removing corrected message (melvo)
* Fix showing message bubble tail only for first message of sender (melvo)

Version 0.12.1
--------------
Released: 2025-03-29

Bugfixes:
* Do not highlight unpinned chats when pinned chat is moved (melvo)
* Fix deleting/sending voice messages (melvo)
* Fix crash during login (melvo)
* Fix opening chat again after going back to chat list on narrow window (melvo)
* Increase toolbar height to fix avatar not being recognizable (melvo)
* Fix width of search bar above chat list to take available space while showing all buttons (melvo)
* Fix storing changed password (melvo)
* Fix setting custom host/port for account registration (melvo)
* Fix crash on chat removal (fazevedo)
* Move device switching options into account details to fix long credentials not being shown and login QR code being temporarily visible on opening dialog (melvo)
* Allow setting new password on error to fix not being able to log in after changing password via other device (melvo)

Version 0.12.0
--------------
Released: 2025-03-21

Features:
* Use square selection to crop avatars (fazevedo)
* Use background with rounded corners for chat list items (melvo)
* Remove colored availability indicator from chat list item (melvo)
* Display group chat participant picker above text cursor in large windows (melvo)
* Do not allow to enter/send messages without visible characters (melvo)
* Remove leading/trailing whitespace from exchanged messages (melvo)
* Ignore received messages without displayable content if they cannot be otherwise processed (melvo)
* Allow to show/hide buttons to pin/move chat list items (melvo)

Bugfixes:
* Fix style for Flatpak (melvo)
* Fix displaying video thumbnails and opening files for Flatpak (melvo)
* Fix message reaction details not opening a second time (melvo)
* Fix opening contact addition view on receiving XMPP URIs (melvo)
* Fix format of text following emojis (melvo)
* Fix eliding last message text for chat list item (melvo)
* Fix unit tests (mlaurent, fazevedo, melvo)
* Fix storing downloaded files with unique names (melvo)
* Fix overlay to change/open avatars shown before hovered in account/contact details (melvo)
* Fix verification of moved contacts (fazevedo)
* Fix setting up end-to-end encryption (OMEMO 2) after account migration (melvo)

Notes:
* Kaidan requires KWindowSystem and KDSingleApplication now (mlaurent)
* Kaidan requires KDE Frameworks 6.11 now
* Kaidan requires KQuickImageEditor 0.5 now
* Kaidan requires QXmpp 1.10.3 now

Version 0.11.0
--------------
Released: 2025-02-14

Features:
* Highlight public XMPP provider titles while card is expanded (melvo)
* Round corners of cards and buttons (melvo)
* Add fading in/out hover effect to map previews (melvo)
* Collapse contact profiles by default if they have more than 3 entries (melvo)
* Show colored check mark for delivered messages instead of none to avoid message bubble resizing (melvo)

Bugfixes:
* Fix opening public MUC-based group chats via another XMPP client (melvo)
* Fix playing voice messages and changing playing position (melvo)
* Fix updating message reactions that could not be sent instead of adding them a second time (melvo)
* Fix updating group chat users in user interface (melvo)
* Fix displaying message reaction details (melvo)
* Update filtering contacts by labels even if label list is not open anymore (melvo)
* Fix scrolling media overview (melvo)
* Fix updating draft messages (melvo)

Notes:
* Kaidan requires Qt 6.6 now (mlaurent, melvo, fazevedo, plata)

Version 0.10.1
--------------
Released: 2024-12-14

Bugfixes:
* Fix displaying files of each message in appropriate message bubble (melvo)
* Fix sending fallback messages for clients not supporting XEP-0447: Stateless file sharing (melvo)
* Fix margins within message bubbles (melvo)
* Fix hiding hidden message part (melvo)
* Fix displaying marker for new messages (melvo)

Version 0.10.0
--------------
Released: 2024-12-09

Features:
* Add server address completion (fazevedo)
* Allow to edit account's profile (jbb)
* Store and display delivery states of message reactions (melvo)
* Send pending message reactions after going online (melvo)
* Enable user to resend a message reaction if it previously failed (melvo)
* Open contact addition as page (mobile) or dialog (desktop) (melvo)
* Add option to open chat if contact exists on adding contact (melvo)
* Use consistent page with search bar for searching its content (melvo)
* Add local message removal (taibsu)
* Allow reacting to own messages (melvo)
* Add login option to chat (melvo)
* Display day of the week or "yesterday" for last messages (taibsu, melvo)
* Add media overview (fazevedo, melvo)
* Add contact list filtering by account and labels (i.e., roster groups) (incl. addition/removal) (melvo, tech-bash)
* Add message date sections to chat (melvo)
* Add support for automatic media downloads (fazevedo)
* Add filtering contacts by availability (melvo)
* Add item to contact list on first received direct message (melvo)
* Add support for blocking chat addresses (lnj)
* Improve notes chat (chat with oneself) usage (melvo)
* Place avatar above chat address and name in account/contact details on narrow window (melvo)
* Reload camera device for QR code scanning as soon as it is plugged in / enabled (melvo)
* Provide slider for QR code scanning to adjust camera zoom (melvo)
* Add contact to contact list on receiving presence subscription request (melvo)
* Add encryption key authentication via entering key IDs (melvo)
* Improve connecting to server and authentication (XEP-0388: Extensible SASL Profile (SASL 2), XEP-0386: Bind 2, XEP-0484: Fast Authentication Streamlining Tokens, XEP-0368: SRV records for XMPP over TLS) (lnj)
* Support media sharing with more clients even for sharing multiple files at once (XEP-0447: Stateless file sharing v0.3) (lnj)
* Display and check media upload size limit (fazevedo)
* Redesign message input field to use rounded corners and resized/symbolic buttons (melvo)
* Add support for moving account data to another account, informing contacts and restoring settings for moved contacts (XEP-0283: Moved) (fazevedo)
* Add group chat support with invitations, user listing, participant mentioning and private/public group chat filtering (XEP-0369: Mediated Information eXchange (MIX), XEP-0405: Mediated Information eXchange (MIX): Participant Server Requirements, XEP-0406: Mediated Information eXchange (MIX): MIX Administration, XEP-0407: Mediated Information eXchange (MIX): Miscellaneous Capabilities) (melvo)
* Add button to cancel message correction (melvo)
* Display marker for new messages (melvo)
* Add enhanced account-wide and per contact notification settings depending on group chat mentions and presence (melvo)
* Focus input fields appropriately (melvo)
* Add support for replying to messages (XEP-0461: Message Replies) (melvo)
* Indicate that Kaidan is busy during account deletion and group chat actions (melvo)
* Hide account deletion button if In-Band Registration is not supported (melvo)
* Embed login area in page for QR code scanning and page for web registration instead of opening start page (melvo)
* Redesign onboarding user interface including new page for choosing provider to create account on (melvo)
* Handle various corner cases that can occur during account creation (melvo)
* Update to XMPP Providers v2 (melvo)
* Hide voice message button if uploading is not supported (melvo)
* Replace custom images for message delivery states with regular theme icons (melvo)
* Free up message content space by hiding unneeded avatars and increasing maximum message bubble width (melvo)
* Highlight draft message text to easily see what is not sent yet (melvo)
* Store sent media in suitable directories with appropriate file extensions (melvo)
* Allow sending media with less steps from recording to sending (melvo)
* Add media to be sent in scrollable area above message input field (melvo)
* Display original images (if available) as previews instead of their thumbnails (melvo)
* Display high resolution thumbnails for locally stored videos as previews instead of their thumbnails (melvo)
* Send smaller thumbnails (melvo)
* Show camera status and reload camera once plugged in for taking pictures or recording videos (melvo)
* Add zoom slider for taking pictures or recording videos (melvo)
* Show overlay with description when files are dragged to be dropped on chats for being shared (melvo)
* Show location previews on a map (melvo)
* Open locations in user-defined way (system default, in-app, web) (melvo)
* Delete media that is only captured for sending but not sent (melvo)
* Add voice message recorder to message input field (melvo)
* Add inline audio player (melvo)
* Add context menu entry for opening directory of media files (melvo)
* Show collapsible buttons to send media/locations inside of message input field (melvo)
* Move button for adding hidden message part to new collapsible button area (melvo)

Bugfixes:
* Fix index out of range error in message search (taibsu)
* Fix updating last message information in contact list (melvo)
* Fix multiple corrections of the same message (melvo, taibsu)
* Request delivery receipts for pending messages (melvo)
* Fix sorting roster items (melvo)
* Fix displaying spoiler messages (melvo)
* Fix displaying errors and encryption warnings for messages (melvo)
* Fix fetching messages from server's archive (melvo)
* Fix various encryption problems (melvo)
* Send delivery receipts for caught up messages (melvo)
* Do not hide last message date if contact name is too long (melvo)
* Fix displaying emojis (melvo)
* Fix several OMEMO bugs (melvo)
* Remove all locally stored data related to removed accounts (melvo)
* Fix displaying media preview file names/sizes (melvo)
* Fix disconnecting from server when application window is closed including timeout on connection problems (melvo)
* Fix media/location sharing (melvo)
* Fix handling emoji message reactions (melvo)
* Fix moving pinned chats (fazevedo)
* Fix drag and drop for files and pasting them (melvo)
* Fix sending/displaying media in selected order (lnj, melvo)

Notes:
* Kaidan is REUSE-compliant now
* Kaidan requires Qt 5.15 and QXmpp 1.9 now

Version 0.9.2
-------------
Released: 2024-07-24

Bugfixes:
* Fix file extension for downloads when mime type is empty (lnj)
* Fix file downloads without a source URL could be started (lnj)
* Fix file messages are never marked as sent (lnj)
* Fix message body of previous file selection was used (lnj)
* Fix missing receipt request (for green checkmark) on media messages (lnj)
* Fix outgoing encrypted media messages are displayed as unencrypted (lnj)

Version 0.9.1
-------------
Released: 2023-05-07

Version 0.9.0
-------------
Released: 2023-04-30

Features:
* New message bubble design based on Tok's code (lnj)
* Group messages from same author (lnj)
* Introduce machine-readable DOAP file describing Kaidan's XMPP compliance (melvo)
* New chat background picture (raghu)
* OMEMO 2 support with easy trust management (melvo)
* Read markers (melvo)
* The chat page title can be clicked to open the user's profile now (mbb)
* Support file sharing with multiple files per message, thumbnails and end-to-end encryption (lnj, jbb)
* Restore window position on start (melvo)
* Chat pinning (melvo, tech-bash)
* Emoji message reactions (melvo)
* The message search now also works with messages that are not displayed (taibsu)
* Public group chat search (without group chat support yet) (fazevedo)
* Account settings with ability to change avatar and profile information (taibsu)
* Redesign of settings, redesign of user profiles as sheet instead of page (mbb)
* Store message drafts locally (fazevedo)

Notes:
* We switched from Weblate to the KDE translation system

Version 0.8.0
-------------
Released: 2021-05-28

Features:
* Add typing notifications (XEP-0085: Chat State Notifications) (jbb)
* Add message history syncing (XEP-0313: Message Archive Management) (lnj)
* Window size is restored (melvo)
* The server's website link is displayed if account creation is disabled (melvo)
* Use breeze theme on macOS (jbb)
* Improved user strings & descriptions (melvo)

Version 0.7.0
-------------
Released: 2021-02-02

Features:
* Display client information (name, version, OS) of contacts (jbb, lnj)
* Drag'n'drop for sending files (jbb)
* Allow pasting images from the clipboard (Ctrl+Shift+V) into the chat (jbb)
* Allow inserting newlines using Shift+Enter (jbb)
* Add configuration of custom hostname/port (jbb, melvo)
* Favourite emojis are shown by default now (melvo)
* Search emojis after entering colon (melvo)
* Display connection errors in the global drawer after login (melvo)
* Improved design of media preview sheets (jbb)
* Restructure message sending bar (melvo)

Bugfixes:
* Do not interpret random URLs as files anymore (lnj)
* Fix the style of buttons when using Material style (melvo)
* Fix file dialog and media drawer opening in some cases (melvo)
* Fix opening of the LoginPage when scanning QR code without password (melvo)

Notes:
* Kaidan requires Qt 5.14 now

Version 0.6.0
-------------
Released: 2020-08-20

Features:
* When offline, messages are cached now to be sent later (yugubich)
* It's allowed to also correct other messages than the last one now (yugubich)
* Also pending (unsent) messages can be corrected now (yugubich)
* Chats can be opened from the notifications now (melvo, jbb, cacahueto)
* New option to permanently hide your password in Kaidan (melvo)
* New buttons for easily copying your jid and password (jbb, fazevedo)
* Moved account management pages into the settings (jbb)
* The cursor is moved to the end of the text field when correcting a message now (melvo)
* Scanning QR codes without a password works now and results in only the JID being set (melvo)
* The roster is called contact list now (jbb)
* The resource for the displayed presence is picked with fixed rules now (it was random before which resource is displayed) (lnj)
* Handle notifications differently on GNOME to keep them in the notifications area (melvo)
* Switched to the upstream HTTP File Upload implementation (lnj)
* Code refactoring and partial rewrite of the following classes: Kaidan, ClientWorker, RosterManager, PresenceCache, DownloadManager, TransferCache, QrCodeDecoder (lnj, jbb)

Bugfixes:
* Playback issues in media video preview (fazevedo)
* Messages sent from other of your devices are displayed as they were sent by the chat partner (lnj)
* Notifications are shown persistently on the screen (jbb)
* Roster names are not updated in the database (melvo)
* Roster items are not updated in the model correctly (melvo)
* All sheets contain two headers: It uses the new built-in header property now (jbb)
* Unreadable buttons with white text on with background in some styles (jbb)
* Database version isn't saved correctly (melvo)
* Errors when building with newer ZXing versions (vkrause)

Notes:
* Kaidan requires a C++17-compliant compiler now

Version 0.5.0
-------------
Released: 2020-04-04

Features:
* Add parsing of XMPP URIs (lnj, melvo)
* Add QR code scanning and generation (lnj, jbb, melvo)
* Add contact search (zatrox, lnj)
* Add muting notifications for messages of contacts (zatrox)
* Add renaming contacts (lnj, zatrox, melvo)
* Show user profile information (lnj, jbb)
* Add extended multimedia support (fazevedo)
* Add message search (blue)
* Redesign contact list, text avatar, counter for unread messages, chat page, chat message bubble (melvo)
* Show notifications on Android (melvo, jbb, cacahueto)
* Add option for enabling or disabling an account temporarily (melvo)
* Refactor login screen with hints for invalid credentials and better usage of keyboard keys (melvo)
* Add message quoting (jbb)
* Truncate very long messages to avoid crashing Kaidan or using it to full capacity (jbb)
* Add button with link for issue tracking to about page (melvo)
* Improve messages for connection errors (melvo)
* Add account deletion (melvo, mbb)
* Redesign logo and global drawer banner (melvo, mbb)
* Add onboarding with registration, normal login and QR code login (melvo, lnj, jbb, mbb)
* Add OARS rating (nickrichards)
* Add secondary roster sorting by contact name (lnj)
* Add support for recording audio and video messages (fazevedo)
* Add Kaidan to KDE's F-Droid repository (nicolasfella)
* Improve build scripts for better cross-platform support (jbb, cacahueto, lnj, mauro)
* Refactor code for better performance and stability (lnj, jbb, melvo)
* Add documentation to achieve easier maintenance (melvo, lnj, jbb)

Bugfixes:
* Fix AppImage build (jbb)
* Fix scrolling and item height problems in settings (jbb)

Notes:
* Require Qt 5.12 and QXmpp 1.2
* Drop Ubuntu Touch support due to outdated Qt

Version 0.4.2
-------------
Released: 2020-04-02

Features:
* ChatMessage: Do not display media URLs (lnj)
* ChatMessage: Add media URL copy action (lnj)

Bugfixes:
* Fix roster not cleared when switching account (lnj)
* ChatMessage: Fix copy to clipboard function (lnj)
* Fix scroll indiciator not overlap message on the right edge (fazevedo)
* Fix upload issues (fazevedo)
* ChatPage: Clear message correction when sent (fazevedo)
* Fix roster sorting (lnj)
* Fix buttonTextColor deprecation warnings (sredman)
* Fix build with QXmpp >= 1.0.1 (lnj)

Version 0.4.1
-------------
Released: 2019-07-16

Bugfixes:
* Fix SSL problems for AppImage (lnj)
* Fix connection problems (lnj)
* Keep QXmpp v0.8.3 compatibility (lnj)

Version 0.4.0
-------------
Released: 2019-07-08

Features:
* Show proper notifications using KNotifications (lnj)
* Add settings page for changing passwords (jbb, lnj)
* Add XEP-0352: Client State Indication (gloox/QXmpp) (lnj)
* Add media/file (including GIFs) sharing (lnj, jbb)
* Full back-end rewrite to QXmpp (lnj)
* Implement XEP-0363: HTTP File Upload and UploadManager for QXmpp (lnj)
* Use XEP-0280: Message Carbons from QXmpp (lnj)
* Use XEP-0352: Client State Indication from QXmpp (lnj)
* Check incoming messages for media links (lnj)
* Implement XEP-0308: Last Message Correction (lnj, jbb)
* Make attachments downloadable (lnj)
* Implement XEP-0382: Spoiler messages (xavi)
* Kaidan is now offline usable (lnj)
* Kaidan is able to open xmpp: URIs (lnj)
* New logo (ilyabizyaev)
* Show presence information of contacts (lnj, melvo)
* Add EmojiPicker from Spectral with search and favorites functionality (jbb, fazevedo)
* Highlight links in chat and make links clickable (lnj)
* New about dialog instead of the about page (ilyabizyaev)
* Add image preview in chat and before sending (lnj)
* Send messages on Enter, new line on Ctrl-Enter (ilyabizyaev)
* 'Add contact' is now the main action on the contacts page (lnj)
* Elide contact names and messages in roster (lnj)
* Chat page redesign (ilyabizyaev)
* Display passive notifications when trying to use online actions while offline (lnj)
* Automatically reconnect on connection loss (lnj)
* Contacts page: Display whether online in title (lnj)
* Add different connection error messages (jbb)
* Use QApplication when building with QWidgets (notmart)
* Ask user to approve subscription requests (lnj)
* Remove contact action: Make JIDs bold (lnj)
* Add contact sheet: Ask for optional message to contact (lnj)
* Add empty chat page with help notice to be displayed on start up (jbb)
* Redesign log in page (sohnybohny)
* Add Copy Invitaion URL action (jbb)
* Add 'press and hold' functionality for messages context menu (jbb)
* Add copy to clipboard function for messages (jbb)
* Add mobile file chooser (jbb)
* Highlight the currently opened chat on contacts page (lnj)
* Remove predefined window sizes (lnj)
* Use new Kirigami application header (nicofee)
* Make images open externally when clicked (jbb)
* Use QtQuickCompiler (jbb)
* Display upload progress bar (lnj)
* Add text+color avatars as fallback (lnj, jbb)
* Remove diaspora log in option (lnj)
* Support for Android (ilyabizyaev)
* Support for Ubuntu Touch (jbb)
* Support for MacOS (ilyabizyaev)
* Support for Windows (ilyabizyaev)
* Support for iOS (ilyabizyaev)
* Add KDE Flatpak (jbb)
* Switch Android builds to CMake with ECM (ilyabizyaev)
* Improve Linux AppImage build script (ilyabizyaev)
* Add additional image formats in AppImage (jbb)
* Forget passwords on log out (lnj)
* Append four random chars to resource (lnj)
* Save passwords in base64 instead of clear text (lnj)
* Generate the LICENSE file automatically with all git authors (lnj)
* Store ubuntu touch builds as job artifacts (lnj)
* Add GitLab CI integration (jbb)

Bugfixes:
* Fix blocking of GUI thread while database interaction (lnj)
* Fix TLS connection bug (lnj)
* Don't send notifications when receiving own messages via. carbons (lnj)
* Fix timezone bug of message timestamps (lnj)
* Fix several message editing bugs (lnj)
* Fix black icons (jbb)
* Fix rich text labels in Plasma Mobile (lnj)
* Small Plasma Mobile fixes (jbb)

Version 0.3.2
-------------
Released: 2017-11-25

Features:
* Added AppImage build script (#138) (@JBBgameich)
* Use relative paths to find resource files (#143) (@LNJ2)
* Source directory is only used for resource files in debug builds (#146) (@LNJ2)

Version 0.3.1
-------------
Released: 2017-11-20

Bugfixes:
* Fixed database creation errors (#135, #132) (@LNJ2)
* ChatPage: Fixed recipient's instead of author's avatar displayed (#131, #137) (@LNJ2)

Features:
* Added Travis-CI integration (#133, #134, #136) (@JBBgameich)
* Added Malay translations (#129) (@MuhdNurHidayat)

Version 0.3.0
-------------
Released: 2017-08-15

Features:
* Added XEP-0280: Message Carbons (#117) (@LNJ2)
* Added XEP-0054/XEP-0153: VCard-based avatars (#73, #105, #119) (@LNJ2)
* Added file storage for simply caching all avatars (@LNJ2)
* New roster design - showing round avatars and last message (#118) (@LNJ2)
* New chat page design - showing time, delivery status and round avatars (#123) (@LNJ2)
* Switched to XMPP client library "gloox" (#114) (@LNJ2)
* Rewritten most of the back-end for gloox and partially also restructured it (#114) (@LNJ2)
* (Re)written new LogHandler for gloox (Swiften had this already included) (#114) (@LNJ2)

Version 0.2.3
-------------
Released: 2017-06-19

Bugfixes:
* LoginPage: Remove material shadow (#113) (@JBBgameich)
* Kaidan was crashing since v0.2.2 when inserting a new message to the DB (@LNJ2)

Version 0.2.2
-------------
Released: 2017-06-19

Bugfixes:
* RosterPage: Clear TextFields after closing AddContactSheet (#106) (@JBBgameich)

Features:
* RosterController: Save lastMessage for each contact (#108) (@LNJ2)
* Add database versioning and conversion (#110) (@LNJ2)
* Database: Add new roster row `avatarHash` (#112) (@LNJ2)
* CMake: Add feature summary (#109) (@LNJ2)

Version 0.2.1
-------------
Released: 2017-06-08

Bugfixes:
* Roster page: Fixed style: Now has contour lines and a cool material effect (@LNJ2)

Version 0.2.0
-------------
Released: 2017-06-06

Features:
* Add Roster Editing (#84, #86) (@LNJ2, @JBBgameich)
* Roster refreshes now automatically (#83) (@LNJ2)
* Contacts are now sorted (@LNJ2)
* Add unread message counters (#92, #101) (@LNJ2)
* Add LibNotify-Linux notifications (#90) (@LNJ2)
* Add custom JID resources (#82) (@LNJ2)
* Add XEP-0184: Message Delivery Receipts (@LNJ2)
* Disable stream compression by default (for HipChat/other server compatibility) (@LNJ2)
* GUI: Port to Kirigami 2 (#81) (@JBBgameich)
* User Material/Green Theme per default (@LNJ2)
* Login page: New design with diaspora* login option (#87) (@JBBgameich)
* Chat page: Slightly improved design (@LNJ2)

Bugfixes:
* AboutPage: Fix possible closing of multiple pages (@LNJ2)
