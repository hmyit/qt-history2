#ifndef QACCESSIBLE_H
#define QACCESSIBLE_H

#ifndef QT_H
#include "qstring.h"
#include "qcom.h"
#include "qrect.h"
#endif // QT_H

#if defined(QT_ACCESSIBILITY_SUPPORT)

class QObject;
struct QAccessibleInterface;

class Q_EXPORT QAccessible
{
public:
    enum Event {
	SoundPlayed	    = 0x0001,
	Alert		    = 0x0002,
	ForegroundChanged   = 0x0003,
	MenuStart	    = 0x0004,
	MenuEnd		    = 0x0005,
	PopupMenuStart	    = 0x0006,
	PopupMenuEnd	    = 0x0007,
	DragDropStart	    = 0x000E,
	DragDropEnd	    = 0x000F,
	DialogStart	    = 0x0010,
	DialogEnd	    = 0x0011,
	ScrollingStart	    = 0x0012,
	ScrollingEnd	    = 0x0013,
	ObjectCreated	    = 0x8000,
	ObjectDestroyed	    = 0x8001,
	ObjectShow	    = 0x8002,
	ObjectHide	    = 0x8003,
	ObjectReorder	    = 0x8004,
	Focus		    = 0x8005,
	Selection	    = 0x8006,
	SelectionAdd	    = 0x8007,
	SelectionRemove	    = 0x8008,
	SelectionWithin	    = 0x8009,
	StateChanged	    = 0x800A,
	LocationChanged	    = 0x800B,
	NameChanged	    = 0x800C,
	DescriptionChanged  = 0x800D,
	ValueChanged	    = 0x800E,
	ParentChanged	    = 0x800F,
	HelpChanged	    = 0x80A0,
	DefaultActionChanged= 0x80B0,
	AcceleratorChanged  = 0x80C0
    };

    enum State {
	Normal		= 0x00000000,
	Unavailable	= 0x00000001,
	Selected	= 0x00000002,
	Focused		= 0x00000004,
	Pressed		= 0x00000008,
	Checked		= 0x00000010,
	Mixed		= 0x00000020,
	ReadOnly	= 0x00000040,
	HotTracked	= 0x00000080,
	Default		= 0x00000100,
	Expanded	= 0x00000200,
	Collapsed	= 0x00000400,
	Busy		= 0x00000800,
	Floating	= 0x00001000,
	Marqueed	= 0x00002000,
	Animated	= 0x00004000,
	Invisible	= 0x00008000,
	Offscreen	= 0x00010000,
	Sizeable	= 0x00020000,
	Moveable	= 0x00040000,
	SelfVoicing	= 0x00080000,
	Focusable	= 0x00100000,
	Selectable	= 0x00200000,
	Linked		= 0x00400000,
	Traversed	= 0x00800000,
	MultiSelectable	= 0x01000000,
	ExtSelectable	= 0x02000000,
	AlertLow	= 0x04000000,
	AlertMedium	= 0x08000000,
	AlertHigh	= 0x10000000,
	Protected	= 0x20000000,
	Valid		= 0x3fffffff
    };

    enum Role {
	NoRole		= 0x00000000,
	TitleBar	= 0x00000001,
	MenuBar		= 0x00000002,
	ScrollBar	= 0x00000003,
	Grip		= 0x00000004,
	Sound		= 0x00000005,
	Cursor		= 0x00000006,
	Caret		= 0x00000007,
	AlertMessage	= 0x00000008,
	Window		= 0x00000009,
	Client		= 0x0000000A,
	PopupMenu	= 0x0000000B,
	MenuItem	= 0x0000000C,
	ToolTip		= 0x0000000D,
	Application	= 0x0000000E,
	Document	= 0x0000000F,
	Pane		= 0x00000010,
	Chart		= 0x00000011,
	Dialog		= 0x00000012,
	Border		= 0x00000013,
	Grouping	= 0x00000014,
	Separator	= 0x00000015,
	ToolBar		= 0x00000016,
	StatusBar	= 0x00000017,
	Table		= 0x00000018,
	ColumnHeader	= 0x00000019,
	RowHeader	= 0x0000001A,
	Column		= 0x0000001B,
	Row		= 0x0000001C,
	Cell		= 0x0000001D,
	Link		= 0x0000001E,
	HelpBalloon	= 0x0000001F,
	Character	= 0x00000020,
	List		= 0x00000021,
	ListItem	= 0x00000022,
	Outline		= 0x00000023,
	OutlineItem	= 0x00000024,
	PageTab		= 0x00000025,
	PropertyPage	= 0x00000026,
	Indicator	= 0x00000027,
	Graphic		= 0x00000028,
	StaticText	= 0x00000029,
	Text		= 0x0000002A,  // Editable, selectable, etc.
	PushButton	= 0x0000002B,
	CheckBox	= 0x0000002C,
	RadioButton	= 0x0000002D,
	ComboBox	= 0x0000002E,
	DropLest	= 0x0000002F,
	ProgressBar	= 0x00000030,
	Dial		= 0x00000031,
	HotkeyField	= 0x00000032,
	Slider		= 0x00000033,
	SpinButton	= 0x00000034,
	Diagram		= 0x00000035,
	Animation	= 0x00000036,
	Equation	= 0x00000037,
	ButtonDropDown	= 0x00000038,
	ButtonMenu	= 0x00000039,
	ButtonDropGrid	= 0x0000003A,
	Whitespace	= 0x0000003B,
	PageTabList	= 0x0000003C,
	Clock		= 0x0000003D
    };

    enum NavDirection {
	NavDirectionMin	= 0x00000000,
	NavUp		= 0x00000001,
	NavDown		= 0x00000002,
	NavLeft		= 0x00000003,
	NavRight	= 0x00000004,
	NavNext		= 0x00000005,
	NavPrevious	= 0x00000006,
	NavFirstChild	= 0x00000007,
	NavLastChild	= 0x00000008,
	NavDirectionMax	= 0x00000009
    };

    static QAccessibleInterface *accessibleInterface( QObject * );
};

// {EC86CB9C-5DA0-4c43-A739-13EBDF1C6B14}
#define IID_QAccessible QUuid( 0xec86cb9c, 0x5da0, 0x4c43, 0xa7, 0x39, 0x13, 0xeb, 0xdf, 0x1c, 0x6b, 0x14 )

struct Q_EXPORT QAccessibleInterface : public QAccessible, public QUnknownInterface
{
    // navigation and hierarchy
    virtual QAccessibleInterface* hitTest( int x, int y, int *who ) const = 0;
    virtual QRect	location( int who ) const = 0;
    virtual QAccessibleInterface* navigate( NavDirection direction, int *target ) const = 0;
    virtual int		childCount() const = 0;
    virtual QAccessibleInterface* child( int who ) const = 0;
    virtual QAccessibleInterface* parent() const = 0;

    // descriptive properties and methods
    virtual bool	doDefaultAction( int who ) = 0;
    virtual QString	defaultAction( int who ) const = 0;
    virtual QString	description( int who ) const = 0;
    virtual QString	help( int who ) const = 0;
    virtual QString	accelerator( int who ) const = 0;
    virtual QString	name( int who ) const = 0;
    virtual QString	value( int who ) const = 0;
    virtual Role	role( int who ) const = 0;
    virtual State	state( int who ) const = 0;

    // selection and focus
    virtual QAccessibleInterface *hasFocus( int *who ) const = 0;
/*
    virtual void	select( int how, int who ) = 0;
    virtual int		selection() const = 0;
*/
};

class Q_EXPORT QAccessibleObject : public QAccessibleInterface
{
public:
    QAccessibleObject( QObject *object );
    virtual ~QAccessibleObject();

    void queryInterface( const QUuid &, QUnknownInterface** );
    ulong addRef();
    ulong release();

    QObject *object() const;

private:
    ulong ref;
    QObject *object_;
};

#endif //QT_ACCESSIBILITY_SUPPORT

#endif //QACCESSIBLE_H
