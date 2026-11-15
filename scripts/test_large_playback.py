"""Offline large-playlist UI: full load, chunk upload, cancel and explicit fallback."""
import json
from test_library_sync import ROOT,run,state

START='''wait 800
open spotify:playlist:mock08
wait 3000
state loaded.txt
click 820 280
wait 200
click 780 210
wait 200
click 260 218
'''
def main():
    out=run('large-playback',{'VAM_MOCK_LARGE_PLAYLIST':'5000','VAM_MOCK_FAST_PAGES':'1'},START+'''wait 2000
state playing.txt
key Right
wait 200
state next.txt
quit
''')
    loaded=state(out,'loaded');assert loaded['loaded']==loaded['total']=='5000'
    playing=state(out,'playing')
    assert playing['sort']=='1' and playing['order_pending']=='0' and playing['original_fallback']=='0'
    assert playing['playback']!='0'
    failed=run('large-fallback',{'VAM_MOCK_LARGE_PLAYLIST':'5000','VAM_MOCK_FAST_PAGES':'1','VAM_MOCK_ORDER_FAIL':'1'},START+'''wait 400
state fallback.txt
shot fallback.bmp
click 370 330
wait 800
state original.txt
quit
''')
    assert state(failed,'fallback')['original_fallback']=='1' and state(failed,'fallback')['playback']=='0'
    assert state(failed,'original')['original_fallback']=='0' and state(failed,'original')['playback']!='0'
    record=dict(status='passed',scope='5000-song synthetic frontend/mock; no Spotify audio',
                assertions=['automatic complete 5000-song load','sorted playback through chunk upload',
                            'no silent order change on upload failure','explicit original-order fallback'])
    (out/'verification.json').write_text(json.dumps(record,indent=2)+'\n');print(json.dumps(record,indent=2))
if __name__=='__main__':main()
