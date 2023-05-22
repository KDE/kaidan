# SPDX-FileCopyrightText: 2022 Albert Astals Cid <aacid@kde.org>
#
# SPDX-License-Identifier: CC0-1.0

$EXTRACT_TR_STRINGS `find . -name \*.cpp -o -name \*.h -o -name \*.qml` -o $podir/kaidan_qt.pot
