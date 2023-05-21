// SPDX-FileCopyrightText: 2017 QZXing authors
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: Apache-2.0

/*
 * Copyright 2017 QZXing authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QByteArray>
#include <QSize>
#include <QVideoFrame>
class QImage;

/**
 * video frame which may contain a QR code
 */
class QrCodeVideoFrame
{
public:
	/**
	 * Instantiates an empty video frame
	 */
	QrCodeVideoFrame()
	    : m_size{0,0},
	      m_pixelFormat{QVideoFrame::Format_Invalid}
	{}

	/**
	 * Sets the frame.
	 *
	 * @param frame frame to be set
	 */
	void setData(QVideoFrame &frame);

	/**
	 * Converts a given image to a grayscale image.
	 *
	 * @return grayscale image
	 */
	QImage *toGrayscaleImage();

	/**
	 * @return content of the frame which may contain a QR code
	 */
	QByteArray data() const;

	/**
	 * @return size of the frame
	 */
	QSize size() const;

	/**
	 * @return format of the frame
	 */
	QVideoFrame::PixelFormat pixelFormat() const;

private:
	QByteArray m_data;
	QSize m_size;
	QVideoFrame::PixelFormat m_pixelFormat;
};
