var EUI = require('eui');
var path = './videos';
var patterns = ['*.mp*'];

VideoController = EUI.VideoController({
  extractPathsFromFileModel: function(model){
    array = [];
    for (var i = 0, j = model.length; i < j; i++){
      if (model.itemAtIndex(i).isFile)
        array.push(model.itemAtIndex(i).path);
    }

    return array;
  },
  init: function(model, index) {
    this.title = model.itemAtIndex(index).name;
    this.model = model;
    this.index = index;
  },
  toolbarItems: function() {

    var items = ['share'];

    if (this.length > 1) {
      items.push('left');
      items.push('right');
    }
    return items;
  },
  selectedToolbarItem: function(item) {
    switch (item) {
    case 'share':
      EUI.Routing.share('video/mp4', this.model.itemAtIndex(this.index).path);
      break;
    case 'left':
      this.setVideo(this.index - 1);
      break;
    case 'right':
      this.setVideo(this.index + 1);
      break;
    }
  },
  itemAtIndex: function(index) {
    return this.model.itemAtIndex(index).path;
  }
});

VideoCollectionController = EUI.GridController({
  init: function(item, patterns) {
    this.title = item.name;
    this.model = new EUI.FileModel(item.path, patterns);
    print(item.path, this.model.length);
  },

  toolbarItems: [ "share" ],
  selectedToolbarItem: function(item) {
    switch (item) {
    case 'share':
      EUI.Routing.share('video/*', this.model.path);
      break;
    }
  },
  itemAtIndex: function(index) {
    var item = this.model.itemAtIndex(index);
    print(item.name);
    return {
      text: item.name,
      //TODO show an icon
      icon: null
    };
  },
  selectedItemAtIndex: function(index) {
    this.pushController(new VideoController(this.model, index));
  }
});

VideoListController = EUI.ListController({
  patterns: patterns,
  model: new EUI.FileModel(path, this.patterns),
  title: 'Video Collections',
  selectedItemAtIndex: function(index) {
    var item = this.model.itemAtIndex(index);

    if (!item.isFile)
      this.pushController(new VideoCollectionController(item, this.patterns));
  },
  itemAtIndex: function(index) {

    var item = this.model.itemAtIndex(index);
    return {
      //TODO show an icon
      icon: null,
      text: item.name
    };
  }
});

EUI.app(new VideoListController());
