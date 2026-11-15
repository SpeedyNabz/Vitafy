"""Exercise the same in-app keyboard used on Vita; no native system overlay."""
import json
from PIL import Image
from test_library_sync import ROOT,run,state

def main():
    out=run('search-keyboard',{'VAM_TOUCH_KEYBOARD':'1'},'''wait 800
click 34 128
wait 200
click 400 35
wait 250
state opened.txt
shot keyboard.bmp
click 103 217
click 618 281
click 430 281
click 714 153
click 244 153
click 524 281
click 432 153
click 848 500
wait 600
state committed.txt
click 400 35
wait 200
click 860 420
click 92 500
wait 200
state cancelled.txt
click 400 35
wait 200
key Down
wait 100
key Return
wait 100
key Escape
wait 200
state controller.txt
click 184 27
wait 300
state defaults.txt
shot settings.bmp
drag 860 410 860 200
wait 400
wheel 1
wait 200
click 860 34
wait 200
state medium.txt
click 860 34
wait 200
state low.txt
shot low.bmp
click 860 34
wait 200
state best.txt
click 550 206
wait 200
state budget.txt
click 300 206
wait 400
state cleared.txt
quit
''')
    assert state(out,'opened')['typing']=='1'
    assert state(out,'committed')['query']=='ambient' and state(out,'committed')['typing']=='0'
    assert state(out,'cancelled')['query']=='ambient' and state(out,'cancelled')['typing']=='0'
    assert state(out,'controller')['typing']=='0' and state(out,'controller')['tab']=='1'
    assert state(out,'defaults')['dim_after']==state(out,'defaults')['off_after']=='0'
    assert state(out,'medium')['quality']=='160'
    assert state(out,'low')['quality']=='96'
    assert state(out,'best')['quality']=='0'
    assert state(out,'budget')['metadata_mb']=='16'
    assert state(out,'cleared')['cache_pending']=='0'
    for name in ('keyboard','settings','low'):
        with Image.open(out/(name+'.bmp')) as im:im.save(out/(name+'.png'))
    record=dict(status='passed',scope='same portable keyboard and settings code under desktop/mock; native acceptance pending',
        assertions=['touch key entry and commit','cancel preserves query','controller input and Back',
                    'timer defaults Never','Best/Medium/Low quality selection','browse-cache budget and acknowledged clear'])
    (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n')
    print(json.dumps(record,indent=2))

if __name__=='__main__':main()
