/*
 * This source file is part of MyGUI. For the latest info, see http://mygui.info/
 * Distributed under the MIT License
 * (See accompanying file COPYING.MIT or copy at http://opensource.org/licenses/MIT)
 */

#include "MyGUI_Precompiled.h"
#include "MyGUI_MenuItem.h"

namespace MyGUI
{

	MenuItem::MenuItem() :
		mOwner(nullptr),
		mMinSize(10, 10),
		mCheck(nullptr),
		mCheckValue(false),
		mHotkey(nullptr)
	{
	}

	void MenuItem::initialiseOverride()
	{
		Base::initialiseOverride();

		// FIXME проверить смену скина ибо должно один раз вызываться
		Widget* parent = getParent();
		MYGUI_ASSERT(parent, "MenuItem must have parent MenuControl");
		if (!parent->isType<MenuControl>())
		{
			Widget* client = parent;
			parent = client->getParent();
			MYGUI_ASSERT(parent, "MenuItem must have parent MenuControl");
			MYGUI_ASSERT(parent->getClientWidget() == client, "MenuItem must have parent MenuControl");
			MYGUI_ASSERT(parent->isType<MenuControl>(), "MenuItem must have parent MenuControl");
		}
		mOwner = parent->castType<MenuControl>();

		///@wskin_child{MenuItem, Widget, Check} Галочка для отмеченного состояния.
		assignWidget(mCheck, "Check");
		assignWidget(mHotkey,"Hotkey");

		//if (isUserString("MinSize"))
		//	mMinSize = IntSize::parse(getUserString("MinSize"));

		//FIXME нам нужен фокус клавы
		setNeedKeyFocus(true);

		updateCheck();
		updateHotkey();
	}

	void MenuItem::shutdownOverride()
	{
		// FIXME проверить смену скина ибо должно один раз вызываться
		mOwner->_notifyDeleteItem(this);

		Base::shutdownOverride();
	}

	void MenuItem::onWidgetCreated(Widget* _widget)
	{
		Base::onWidgetCreated(_widget);

		MenuControl* child = _widget->castType<MenuControl>(false);
		if (child != nullptr)
		{
			mOwner->_wrapItemChild(this, child);
		}
	}

	void MenuItem::setCaption(const UString& _value)
	{
		Button::setCaption(_value);
		mOwner->_notifyUpdateName(this);
	}

	const UString& MenuItem::getItemName()
	{
		return mOwner->getItemName(this);
	}

	void MenuItem::setItemName(const UString& _value)
	{
		mOwner->setItemName(this, _value);
	}

	void MenuItem::setItemData(Any _data)
	{
		mOwner->setItemData(this, _data);
	}

	void MenuItem::removeItem()
	{
		mOwner->removeItem(this);
	}

	void MenuItem::setItemId(const std::string& _id)
	{
		mOwner->setItemId(this, _id);
	}

	const std::string& MenuItem::getItemId()
	{
		return mOwner->getItemId(this);
	}

	size_t MenuItem::getItemIndex()
	{
		return mOwner->getItemIndex(this);
	}

	MenuControl* MenuItem::createItemChild()
	{
		return mOwner->createItemChild(this);
	}

	void MenuItem::setItemType(MenuItemType _type)
	{
		mOwner->setItemType(this, _type);
	}

	MenuItemType MenuItem::getItemType()
	{
		return mOwner->getItemType(this);
	}

	void MenuItem::setItemChildVisible(bool _visible)
	{
		mOwner->setItemChildVisible(this, _visible);
	}

	MenuControl* MenuItem::getItemChild()
	{
		return mOwner->getItemChild(this);
	}

	void MenuItem::setPropertyOverride(const std::string& _key, const std::string& _value)
	{
		/// @wproperty{MenuItem, MenuItemId, string} Идентификатор строки меню.
		if (_key == "MenuItemId")
			setItemId(_value);

		/// @wproperty{MenuItem, MenuItemType, MenuItemType} Тип строки меню.
		else if (_key == "MenuItemType")
			setItemType(utility::parseValue<MenuItemType>(_value));

		/// @wproperty{MenuItem, MenuItemChecked, bool} Отмеченное состояние строки меню.
		else if (_key == "MenuItemChecked")
			setItemChecked(utility::parseValue<bool>(_value));

		else
		{
			Base::setPropertyOverride(_key, _value);
			return;
		}

		eventChangeProperty(this, _key, _value);
	}

	MenuControl* MenuItem::getMenuCtrlParent()
	{
		return mOwner;
	}

	IItemContainer* MenuItem::_getItemContainer()
	{
		return mOwner;
	}

	IntSize MenuItem::_getContentSize()
	{
		ISubWidgetText* text = getSubWidgetText();
		if (text == nullptr)
			return mMinSize;

		IntSize size;
		if( mHotkey )
		{
			size = mHotkey->getTextSize();
			//size.width *= 2;
			size.height = 0;
		}
		return text->getTextSize() + (getSize() - text->getSize()) + size;
	}

	void MenuItem::updateHotkey()
	{
		if( mHotkey )
		{
			std::string key = getUserString("hotkey");
			if( key.empty() )
			{
				mHotkey->setCaption(key);
			}
			else
			{
			//	MyGUI::UString s="#FFFF00#<B>[#<I>";
			//	s.append(getUserString("hotkey"));
			//	s.append("#<B>]");
				MyGUI::UString s = getUserString("hotkey");
				mHotkey->setCaption(s);
				MyGUI::IntSize size = mHotkey->getTextSize();
				MyGUI::IntCoord co = mHotkey->getCoord();
				MyGUI::IntSize ws = getSize();
				mHotkey->setCoord( ws.width-size.width-4,co.top,size.width,co.height );
			}
		}
	}

	void MenuItem::updateCheck()
	{
		if (mCheck != nullptr)
			mCheck->setVisible(mCheckValue);
	}

	bool MenuItem::getItemChecked() const
	{
		return mCheckValue;
	}

	void MenuItem::setItemChecked(bool _value)
	{
		mCheckValue = _value;
		updateCheck();
	}

	void MenuItem::setUserString(const std::string& _key, const std::string& _value)
	{
		UserData::setUserString(_key,_value);
		if( _key == "hotkey" )
			updateHotkey();
	}

	bool MenuItem::clearUserString(const std::string& _key)
	{
		bool bret = UserData::clearUserString(_key);
		if( _key == "hotkey" )
			updateHotkey();
		return bret;
	}

	void MenuItem::clearUserStrings()
	{
		UserData::clearUserStrings();
		updateHotkey();
	}
} // namespace MyGUI
