/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#ifndef __MYGUI_I_FONT_H__
#define __MYGUI_I_FONT_H__

#include "MyGUI_Prerequest.h"
#include "MyGUI_ISerializable.h"
#include "MyGUI_IResource.h"
#include "MyGUI_FontData.h"

namespace MyGUI
{

	class ITexture;

	class MYGUI_EXPORT IFont :
		public IResource
	{
		MYGUI_RTTI_DERIVED( IFont )
	public:
		enum Style
		{
			Normal = 0,
			Bold = 1,
			Italic = 2,
			BoldItalic =3
		};
	public:
		IFont() { }
		virtual ~IFont() { }

		virtual GlyphInfo* getGlyphInfo(Char _id) = 0;

		virtual ITexture* getTextureFont() = 0;

		virtual int getDefaultHeight() = 0;

		/*
			为了支持粗体和斜体，这里加入一个函数setStyle
		*/
		virtual void setStyle( IFont::Style _style ) = 0;
	};

} // namespace MyGUI

#endif // __MYGUI_I_FONT_H__
