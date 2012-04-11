var elm = require('elm');

var EXPAND_BOTH = { x: 1.0, y: 1.0 };
var FILL_BOTH = { x: -1.0, y: -1.0 };

var logo_icon = elm.Icon({
    image: elm.datadir + "data/images/logo_small.png"
});

var logo_icon_unscaled = elm.Icon({
    image: elm.datadir + "data/images/logo_small.png",
    resizable_up: false,
    resizable_down: false
});

var w = elm.Window({
    title: "Check box demo",
    elements: {
        the_background: elm.Background({
            weight: EXPAND_BOTH,
            resize: true
        }),
        the_box: elm.Box({
            weight: EXPAND_BOTH,
            resize: true,
            elements: {
                icon_sized_check: elm.Check({
                    label: "Icon sized to check",
                    weight: { x: -1.0, y: -1.0 },
                    align: { x: -1.0, y: 0.5 },
                    icon: logo_icon,
                    state: true
                }),
                toggle1: elm.Check({
                    style: "toggle",
                    label: "Spades",
                    onlabel: "Numbers",
                    offlabel: "Alphabets",
                    icon: logo_icon,
                    weight: { x: -1.0, y: -1.0 },
                    align: { x: -1.0, y: 0.5 },
                    on_changed: function(arg) {
                        print("Numbers or Alphabets\n" + elm.datadir);
                    }
                }),
                icon_check: elm.Check({
                    label: "Icon no scale",
                    icon: logo_icon_unscaled
                }),
                label_check: elm.Check({
                    label: "Label Only"
                }),
                disabled_check: elm.Check({
                    label: "Disabled check",
                    weight: { x: -1.0, y: -1.0 },
                    align: { x: -1.0, y: 0.5 },
                    icon: logo_icon,
                    state: true,
                    disabled: true
                }),
                disabled_icon_only: elm.Check({
                    icon: logo_icon_unscaled
                }),
            }
        })
    }
});

var e = elm.realise(w);
