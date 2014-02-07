const St = imports.gi.St;
const Main = imports.ui.main;
const Tweener = imports.ui.tweener;
const Mainloop = imports.mainloop;
const Lang = imports.lang;
const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;



let text, label, ctr = 0;


function PanelButton() {
    this._init();
}
 
PanelButton.prototype = {
    _init: function() {
        this.actor = new St.Label({ text: "HelloWorld Button" });
        Main.panel._rightBox.insert_child_at_index(this.actor, 0) //add(this.actor, { y_fill: true , x_fill:true, x_align: St.Align.START});
 
	_update()
    },
 
    _onDestroy: function() {}
};

function _update(text) {
	  if(!text) {
	    Mainloop.timeout_add(1000, _update);
	    cat("/tmp/status")
	  } else {    
	    ctr += 1
	    if (label) {
	      label.actor.text = " " + text
	    }
	  }
}

function cat(filename) {
    let f = Gio.file_new_for_path(filename);
    f.load_contents_async(null, function(f, res) {
        let contents;
        try {
            contents = f.load_contents_finish(res)[1];
        } catch (e) {
            log("*** ERROR: " + e.message);
            return;
        }
        _update(contents)
    });
}

function init() {
    label = new PanelButton();
}

function enable() {
    Main.panel._rightBox.insert_child_at_index(label.actor, 0);
}

function disable() {
    Main.panel._rightBox.remove_child(label.actor);
}
