#include "eflpp_etk.h"

/* EFL */
#include <Ecore_Evas.h>

/* STD */
#include <iostream>
#include <assert.h>
using namespace std;

namespace efl {

EtkApplication* EtkApplication::_instance = 0;

//==========================================================================//
// EwlObject
//==========================================================================//

EtkObject::EtkObject( EtkObject* parent, const char* type, const char* name )
          :_o( 0 )
{
    cout << "EtkObject::EtkObject - generating object '" << ( name ? name : "<unknown>" ) << "' (" << type << ")" << endl;
    if ( strcmp( type, "TopLevelWidget" ) == 0 )
    {
        assert( not "implemented" );
        //_o = ETK_OBJECT( etk_widget_new() );
    }
    else if ( strcmp( type, "Embed" ) == 0 )
    {
        // already created
    }
    else if ( strcmp( type, "Window" ) == 0 )
    {
        _o = ETK_OBJECT( etk_window_new() );
    }
    else if ( strcmp( type, "Button" ) == 0 )
    {
        _o = ETK_OBJECT( etk_button_new() );
    }
    else
        assert( not "a known widget" );
}

EtkObject::~EtkObject()
{
}

void EtkObject::resize( const Size& size )
{
    etk_widget_size_request_set( ETK_WIDGET(_o), size.width(), size.height() );
}

void EtkObject::setFillPolicy( )
{
}

void EtkObject::setAlignment( )
{
}

//==========================================================================//
// EtkWidget
//==========================================================================//

EtkWidget::EtkWidget( EtkObject* parent, const char* type, const char* name )
    :EtkObject( parent, type, name )
{
}

EtkWidget::~EtkWidget()
{
}

void EtkWidget::show()
{
    etk_widget_show( ETK_WIDGET(_o) );
}

void EtkWidget::showAll()
{
    etk_widget_show_all( ETK_WIDGET(_o) );
}

void EtkWidget::hide()
{
    etk_widget_hide( ETK_WIDGET(_o) );
}

void EtkWidget::hideAll()
{
    etk_widget_hide_all( ETK_WIDGET(_o) );
}

bool EtkWidget::isVisible() const
{
    return etk_widget_is_visible( ETK_WIDGET(_o) );
}

void EtkWidget::setVisibilityLock( bool b )
{
    // hack: [audifahrer]
    //etk_widget_visibility_locked_set( ETK_WIDGET(_o), b );
}

bool EtkWidget::visibilityLock() const
{
    // hack: [audifahrer]
    //return etk_widget_visibility_locked_get( ETK_WIDGET(_o) );
    return false;
}

//==========================================================================//
// EtkContainer
//==========================================================================//

EtkContainer::EtkContainer( EtkObject* parent, const char* type, const char* name )
    :EtkWidget( parent, type, name )
{
}

EtkContainer::~EtkContainer()
{
}

void EtkContainer::appendChild( EtkWidget* child )
{
   etk_container_add( ETK_CONTAINER(_o), ETK_WIDGET(child->obj()) );
}

void EtkContainer::setBorderWidth( int width )
{
   etk_container_border_width_set( ETK_CONTAINER(_o), 5);
}

//==========================================================================//
// EtkTopLevelWidget
//==========================================================================//

EtkTopLevelWidget::EtkTopLevelWidget( EtkObject* parent, const char* type, const char* name )
    :EtkContainer( parent, type, name )
{
}

EtkTopLevelWidget::~EtkTopLevelWidget()
{
}

//==========================================================================//
// EtkEmbed
//==========================================================================//

EtkEmbed::EtkEmbed( EvasCanvas* canvas, EtkObject* parent, const char* type, const char* name )
    :EtkTopLevelWidget( parent, type, name )
{
        _o = ETK_OBJECT( etk_embed_new( canvas->obj()) );
}

EtkEmbed::~EtkEmbed()
{
}

void EtkEmbed::setFocus( bool b )
{
    //ewl_embed_focus_set( EWL_EMBED( _o ), b );
}

//==========================================================================//
// EvasEtk
//==========================================================================//

EvasEtk::EvasEtk( EtkEmbed* ewlobj, EvasCanvas* canvas, const char* name )
    :EvasObject( canvas )

{
    o = etk_embed_object_get( ETK_EMBED(ewlobj->obj()) );
}

EvasEtk::~EvasEtk()
{
}

//==========================================================================//
// EtkWindow
//==========================================================================//

EtkWindow::EtkWindow( EtkObject* parent, const char* type, const char* name )
    :EtkTopLevelWidget( parent, type, name )
{
}

EtkWindow::~EtkWindow()
{
}

void EtkWindow::setTitle( const char* title )
{
    etk_window_title_set( ETK_WINDOW( _o ), strdup( title ) );
}

//==========================================================================//
// EtkBox
//==========================================================================//

EtkBox::EtkBox( EtkObject* parent, const char* type, const char* name )
    :EtkContainer( parent, type, name )
{
    //_o = ETK_OBJECT( etk_box_new() );
}

EtkBox::~EtkBox()
{
}

//==========================================================================//
// EtkHBox
//==========================================================================//

EtkHBox::EtkHBox( EtkObject* parent, const char* type, const char* name )
    :EtkBox( parent, type, name )
{
    //ewl_box_orientation_set( EWL_BOX(_o), EWL_ORIENTATION_HORIZONTAL );
}

EtkHBox::~EtkHBox()
{
}

//==========================================================================//
// EtkVBox
//==========================================================================//

EtkVBox::EtkVBox( EtkObject* parent, const char* type, const char* name )
    :EtkBox( parent, type, name )
{
    //ewl_box_orientation_set( EWL_BOX(_o), EWL_ORIENTATION_VERTICAL );
}

EtkVBox::~EtkVBox()
{
}

//==========================================================================//
// EtkButton
//==========================================================================//

EtkButton::EtkButton( EtkObject* parent, const char* type, const char* name )
    :EtkBox( parent, type, name )
{
    setText( name ? name : "unnamed" );
}

EtkButton::EtkButton( const char* text, EtkObject* parent, const char* type, const char* name )
    :EtkBox( parent, type, name )
{
    setText( text );
}

EtkButton::~EtkButton()
{
}

void EtkButton::setText( const char* text )
{
    etk_button_label_set( ETK_BUTTON(_o), const_cast<char*>( text ) );
}

//===============================================================================================
// EtkApplication
//===============================================================================================

EtkApplication::EtkApplication( int argc, const char **argv, const char* name )
    :_name( name )
{
    if ( EtkApplication::_instance )
    {
        cout << "FATAL: EtkApplication object already created" << endl;
            ::exit( -1 );
    }
    cout << "EtkApplication::EtkApplication" << endl;
    if ( !etk_init( &argc, const_cast<char***>(&argv) ) )
    {
            cout << "FATAL: Couldn't initialize Etk" << endl;
            ::exit( -1 );
    }
    EtkApplication::_instance = this;
}

EtkApplication::~EtkApplication()
{
    cout << "EtkApplication::~EtkApplication" << endl;
    etk_shutdown();
}

EtkApplication* EtkApplication::application()
{
    if ( !EtkApplication::_instance )
    {
        cerr << "FATAL: EtkApplication object hasn't been created yet" << endl;
            ::exit( -1 );
    }
    else
    {
        return EtkApplication::_instance;
    }
}

void EtkApplication::setMainWindow( EtkWindow* mw )
{
    _mainWindow = mw;
    mw->setTitle( _name );
}

EtkWindow* EtkApplication::mainWindow() const
{
    return _mainWindow;
}

void EtkApplication::exec()
{
    cout << "EtkApplication::exec() - >>> jumping into main loop" << endl;
    etk_main();
    cout << "EtkApplication::exec() - <<< returned from main loop" << endl;
}

void EtkApplication::exit()
{
    //ecore_main_loop_quit();
}

}
