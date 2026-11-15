"""Offline frontend regression for preview.4 filters, dropdowns and full loading."""
import json
from test_library_sync import ROOT, run, state
from PIL import Image

def main():
    out=run('library-controls',{},'''wait 800
click 34 176
wait 400
state all-items.txt
click 270 76
wait 200
state playlists.txt
click 550 76
wait 200
state owned.txt
shot stacked.bmp
click 550 76
click 365 76
wait 200
state albums.txt
click 455 76
wait 200
state artists.txt
click 180 254
wait 200
shot library-sort.bmp
key Escape
wait 200
state closed.txt
open spotify:playlist:mock08
wait 4000
state loaded.txt
shot playlist.bmp
click 820 280
wait 250
state menu.txt
shot sort.bmp
click 780 338
wait 250
state added.txt
click 820 280
wait 200
click 780 338
wait 250
state ascending.txt
click 820 280
wait 200
click 450 280
wait 250
state outside.txt
click 820 280
wait 200
key Escape
wait 200
state escaped.txt
quit
''')
    assert int(state(out,'playlists')['library_matches'])<int(state(out,'all-items')['library_matches'])
    assert state(out,'owned')['category']=='1' and state(out,'owned')['by_you']=='1'
    assert 0<int(state(out,'owned')['library_matches'])<int(state(out,'playlists')['library_matches'])
    assert state(out,'albums')['category']=='2' and int(state(out,'albums')['library_matches'])>0
    assert state(out,'artists')['category']=='3' and int(state(out,'artists')['library_matches'])>0
    assert state(out,'closed')['depth']=='1'
    loaded=state(out,'loaded')
    assert loaded['loaded']==loaded['total']=='320' and loaded['compact']=='0' and loaded['filter']==''
    assert state(out,'menu')['sort_open']=='1'
    assert state(out,'added')['sort']=='5' and state(out,'added')['descending']=='1'
    assert state(out,'ascending')['sort']=='5' and state(out,'ascending')['descending']=='0'
    assert state(out,'outside')['sort_open']=='0' and state(out,'outside')['sort']=='5'
    assert state(out,'escaped')['sort_open']=='0' and state(out,'escaped')['depth']=='2'
    for name in ('stacked','library-sort','playlist','sort'):
        with Image.open(out/(name+'.bmp')) as im:im.save(out/(name+'.png'))
    record=dict(status='passed',scope='offline real frontend/mock; no Spotify audio',
        assertions=['category + By you stack; Downloaded removed',
                    'album and artist categories','sidebar sorting dropdown',
                    'all 320 entries load without scrolling or Load all','playlist uses plain list with no text filter',
                    'dropdown selection and direction toggle','outside click dismisses without changing sort',
                    'Escape dismisses without navigating'])
    (ROOT/'artifacts/library-sync/library-controls/verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
