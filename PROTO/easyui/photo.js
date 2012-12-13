var EUI = require('eui');
var path = environment['HOME'] + '/Photos';
var patterns = ['*.jpg', '*.jpeg', '*.png'];

/*
 * Displays the photo itself. ImageController will display each
 * photo on model, so we just take care of satellite features,
 * like the slideshow, or hiding the toolbars while showing
 * the photo
 */
PhotoController = EUI.ImageController({
  init: function(model, index, slideshow) {
    this.model = model;
    this.index = index;

    /*
     * Here we setup the slideshow, if the option is enabled.
     */
    if (slideshow) {
      this.chromeVisible = false;
      this.slideshow = setInterval(function() {
        if (this.chromeVisible)
          return;

        this.index = (this.index + 1) % this.length;
        this.setImage(this.index);
      }.bind(this), 5000);
    } else {
      this.chromeVisible = true;
    }
  },
  isFullscreen: function() {
    return !!this.slideshow;
  },
  title: function() {
    return this.chromeVisible ? this.model.itemAtIndex(this.index).name : null;
  },
  didClickView: function() {
    /*
     * When the user clicks the photo, we show (or hide) the toolbar.
     */
    this.chromeVisible = !this.chromeVisible;
    this.evaluateViewChanges();
  },
  willPopController: function() {
    if (this.slideshow)
      clearInterval(this.slideshow);
  },
  toolbarItems: function() {
    /*
     * The toolbar items for this controller are created dynamically,
     * depending on the amount of items in the album (there's no need for
     * prev/next buttons on an album with just one picture in it.)
     */
    if (!this.chromeVisible)
      return [];

    var items = [
      {
        tag: 'share',
        icon: 'mail-send',
        label: 'Share'
      }
    ];

    if (this.length > 1) {
      items.push({
        tag: 'left',
        icon: 'go-previous',
        label: 'Previous'
      });
      items.push({
        tag: 'right',
        icon: 'go-next',
        label: 'Next'
      });
    }
    return items;
  },
  selectedToolbarItem: function(item) {
    clearInterval(this.slideshow);
    switch (item.tag) {
    case 'share':
      var file = this.model.itemAtIndex(this.index);
      EUI.Routing.share('image/jpeg', file.path);
      break;
    case 'left':
      this.setImage(this.index - 1);
      break;
    case 'right':
      this.setImage(this.index + 1);
      break;
    }
  }
});

/*
 * This is the secondary controller: it displays the contents of a photo
 * album.  It is instantiated by the PhotoAlbumController, but instead of
 * presenting items one-per-line, it does so using a grid layout.
 */
PhotoAlbumController = EUI.GridController({
  init: function(item, patterns) {
    this.title = item.name;
    this.model = new EUI.FileModel(item.path, patterns);
  },
  /*
   * In addition to also having a navigation bar, this controller also has a
   * toolbar.  Note that there's no need for a hasToolbar variable, since
   * that's inferred by the toolbarItems variable.  Should there be a need
   * for dynamically-created toolbars, this can be a function that returns
   * an array as well.
   */
  toolbarItems: function() {
    return [
      { tag: "share",	label: 'Share',	icon: 'mail-send' },
      { tag: "play",	label: 'Slideshow', icon: 'media-playback-start' }
    ];
  },
  selectedToolbarItem: function(item) {
    /*
     * Since the toolbar is integrated into EUI, it already knows
     * which function to call when an item is selected. The item
     * is exactly the type that is available in the toolbarItems
     * array.
     */
    switch (item.tag) {
    case 'share':
      /*
       * Share will display a popup with sharing options defined by
       * EUI.Routing.
       */
      EUI.Routing.share('image/*', this.model.path);
      break;
    case 'play':
      this.pushController(new PhotoController(this.model, 0, true));
      break;
    }
  },
  itemAtIndex: function(index) {
    /*
     * This gets the first image (by file name filters, as seen in the
     * FileModel initialization above) for the nth directory.  If it exists,
     * then return an item with that image as the album "cover".
     *
     * Usually this function does not need to be defined: but there's a need
     * to access the first image inside the FileModel to use as this album's
     * cover, so there you go.
     */
    var item = this.model.itemAtIndex(index);
    var picture = {text: item.name, icon: undefined};

    if (item.isFile) {
      picture.icon = item.path;
    } else if (item.isFile == false && item.entries) {
      for (var i = 0; i < item.entries.length; ++i)
        if (item.entries[i].isFile) {
          picture.icon = item.entries[i].path;
          break;
        }
    }

    if (picture.icon)
      delete picture.text;

    return picture;
  },
  selectedItemAtIndex: function(index) {
    var item = this.model.itemAtIndex(index);
    this.pushController(new PhotoController(this.model, index));
  }
});

/*
 * This is the main controller for this application. The user interface will
 * be created as a list (since it is a ListController), listing all
 * directories under ~/Photos which contains at least one image file.  The
 * filtering is made using EUI's FileModel, which abstracts basic file
 * system access.
 *
 * Note that, by using a standard interface, the list controller already
 * knows which methods to call to obtain information about an item, or what
 * to perform when an item is selected.  Since there's no need to massage
 * items from the model, the itemAtIndex() function has been ommitted.
 */
AlbumListController = EUI.ListController({
  /*
   * As soon as this controller is pushed to the root controller, EUI will
   * set up this model to notify this controller when it is ready.  This
   * could be fired on a lot of occasions, including, but not limited to,
   * files being added/removed in that particular directory, or (in other
   * kinds of models) information received from the network.
   * We also use a FilterModel to not display empty directories.
   */
  patterns: patterns,
  model: new EUI.FilterModel (
    new EUI.FileModel(path, this.patterns),
    function(file) {
      return file.isFile || file.entries.length;
    }),
  title: 'Photo Albums',
  selectedItemAtIndex: function(index) {
    /*
     * This will create a drill-down user interface because
     * ``this.hasNavigationBar'' is true.  The pushController method will
     * set the created controller's model to be the selected item in this
     * controller, and the parent to be this controller.
     */
    this.pushController(new PhotoAlbumController(this.model.itemAtIndex(index),
                        this.patterns));
  },
  itemAtIndex: function(index) {
    /*
     * This gets the first image (by file name filters, as seen in the
     * FileModel initialization above) for the nth directory.  If it exists,
     * then return an item with that image as the album "cover".
     *
     * Usually this function does not need to be defined: but there's a need
     * to access the first image inside the FileModel to use as this album's
     * cover, so there you go.
     */
    var item = this.model.itemAtIndex(index);
    var picture = {text: item.name, icon: undefined};

    if (!item.entries)
      return { text: item.name };

    for (var i = 0; i < item.entries.length; ++i) {
      if (item.entries[i].isFile) {
        return {
          text: item.name,
          icon: item.entries[i].path
        };
      }
    }
  }
});

/*
 * EUI.app(c) tells EUI to initialize the application
 * using that particular controller.
 */
EUI.app(new AlbumListController());
