"""Offline regression: real frontend hearts, remote invalidation, failure, history and scoped filtering."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

ROOT=Path(__file__).resolve().parent.parent

def run(name,extra,script):
    out=ROOT/'artifacts/library-sync'/name
    out.mkdir(parents=True,exist_ok=True)
    path=out/'commands.txt';path.write_text(script)
    exe=ROOT/('build/desktop/Release/VitafyPreview.exe' if os.name=='nt' else 'build/desktop/VitafyPreview')
    with tempfile.TemporaryDirectory(prefix='vitafy-sync-') as data:
        env=dict(os.environ,SDL_VIDEODRIVER='dummy',SDL_RENDER_DRIVER='software',VAM_MOCK_INSTANT='1',
                 VAM_DATA_DIR=data+os.sep,VAM_SCRIPT=str(path),**extra)
        for key in ('VAM_MOCK_FREE','VAM_MOCK_STAY_OUT'):env.pop(key,None)
        with (out/'stderr.log').open('w') as log:
            subprocess.run([str(exe)],cwd=out,env=env,stdout=log,stderr=log,timeout=30,check=True,
                           creationflags=getattr(subprocess,'CREATE_NO_WINDOW',0))
    return out

def state(out,name):return dict(line.split('=',1) for line in (out/(name+'.txt')).read_text().splitlines())
def hearts(out,name,expected):
    s=state(out,name)
    assert s['player_saved']==str(expected) and s['row_saved']==str(expected),(name,s)

START='''wait 800
click 80 306
wait 500
click 430 412
wait 700
mock_saved 0
wait 200
state before.txt
'''

def main():
    out=run('success',{},START+'''click 284 502
wait 80
state pending.txt
wait 400
state confirmed.txt
key n
wait 350
click 890 186
wait 80
state song-view.txt
wait 400
key n
wait 350
mock_saved 1
wait 300
state remote-like.txt
mock_saved 0
wait 300
state remote-unlike.txt
click 820 280
wait 200
click 780 210
wait 200
state filtered.txt
shot filtered.bmp
key Escape
wait 250
state back.txt
click 150 220
wait 300
state forward.txt
quit
''')
    hearts(out,'before',0);hearts(out,'pending',1);hearts(out,'confirmed',1)
    hearts(out,'song-view',0);hearts(out,'remote-like',1);hearts(out,'remote-unlike',0)
    assert state(out,'filtered')['sort']=='1' and state(out,'filtered')['filter']==''
    assert state(out,'back')['depth']=='1'
    assert state(out,'forward')['depth']=='2' and state(out,'forward')['sort']=='1'
    delayed=run('delayed',{'VAM_MOCK_DELAY_ECHO':'1'},START+'''click 284 502
wait 80
state pending.txt
mock_saved 1
wait 400
state echoed.txt
quit
''')
    hearts(delayed,'pending',1);hearts(delayed,'echoed',1)
    failed=run('failure',{'VAM_MOCK_FAIL_SAVE':'1'},START+'''click 284 502
wait 80
state pending.txt
wait 500
state failed.txt
quit
''')
    hearts(failed,'pending',1);hearts(failed,'failed',0)
    created=run('create',{},'''wait 800
click 34 176
wait 500
state before.txt
click 830 30
wait 150
text Test list
key Return
wait 700
state created.txt
quit
''')
    assert int(state(created,'created')['playlists'])==int(state(created,'before')['playlists'])+1
    assert state(created,'created')['last_playlist']=='Test list' and state(created,'created')['edit_pending']=='0'
    grid=run('grid',{},'''wait 800
click 34 176
wait 500
state grid.txt
shot grid.bmp
click 350 172
wait 200
state compact-grid.txt
shot compact-grid.bmp
quit
''')
    assert state(grid,'grid')['grid']=='0' and state(grid,'grid')['compact']=='0'
    assert state(grid,'compact-grid')['grid']=='0' and state(grid,'compact-grid')['compact']=='0'
    for name in ('grid','compact-grid'):
        with Image.open(grid/(name+'.bmp')) as im: im.save(grid/(name+'.png'))
    with Image.open(out/'filtered.bmp') as im: im.save(out/'filtered.png')
    record=dict(status='passed',scope='offline frontend/mock; no Spotify account or audio',
        assertions=['playlist/player like sync','Now Playing unlike sync','pending heart survives stale snapshots',
                    'external like/unlike propagation','failed write rollback','playlist has no text filter','back/forward retains sort',
                    'acknowledged playlist creation refreshes library','library uses plain list without presentation switches'])
    (ROOT/'artifacts/library-sync/verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
