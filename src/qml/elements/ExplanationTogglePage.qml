// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

/**
 * This is the base for pages having an explained content and a button for toggling the explanation.
 */
ExplainedContentPage {
	primaryButton.checkable: true
	primaryButton.onClicked: explanationArea.visible = !explanationArea.visible
}
