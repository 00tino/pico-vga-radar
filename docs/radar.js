const EMBED=new URLSearchParams(location.search).has('embed');
if(EMBED) document.body.classList.add('embed');
let AIRPORTS=[], ALINES=[];
const FALLBACK_AP=[['AEP','Buenos Aires',-34.559,-58.416,'AR','Aeroparque','Argentina',''],['EZE','Buenos Aires',-34.822,-58.536,'AR','Ezeiza','Argentina',''],['ASU','Asuncion',-25.24,-57.52,'PY','Silvio Pettirossi','Paraguay',''],['SYD','Sydney',-33.946,151.177,'AU','Kingsford Smith','Australia','']];
const ROUTECACHE={}, APTINDEX={};
let home=null,demoOver=false,srcLabel='';
function hay(a){return (Array.isArray(a)?a.join(' '):'').toLowerCase()}
function findAirport(q){
  const s=(q||'').trim();
  if(!s) return (AIRPORTS[0]||FALLBACK_AP[0]);
  if(s.toUpperCase()==='CASA'&&home) return ['CASA','Casa',home[0],home[1],'AR','Casa','',''];
  const list=AIRPORTS.length?AIRPORTS:FALLBACK_AP;
  const up=s.toUpperCase();
  const code=up.slice(0,3);
  const exact=list.find(a=>a[0]===code && (up.length===3 || up.startsWith(a[0])));
  if(exact && (up===exact[0] || /^[A-Z]{3}\b/.test(up))) return exact;
  const low=s.toLowerCase();
  return list.find(a=>hay(a).includes(low)) || list[0];
}
function currentCode(){return findAirport(document.getElementById('preset').value||'AEP')[0]}
function bindSuggest(input,listEl,getRows,fmt,onPick){
  function show(q){
    const s=(q||'').toLowerCase().trim();
    const rows=getRows();
    const hits=(!s?rows.slice(0,8):rows.filter(r=>hay(r).includes(s)).slice(0,12));
    listEl.innerHTML=hits.map(r=>'<button type="button" data-v="'+r[0]+'">'+fmt(r)+'</button>').join('') || '<button type="button" disabled>Sin resultados</button>';
    listEl.style.display='block';
  }
  input.addEventListener('input',()=>show(input.value));
  input.addEventListener('focus',()=>show(input.value));
  listEl.addEventListener('mousedown',e=>{const b=e.target.closest('button'); if(!b||!b.dataset.v) return; onPick(b.dataset.v,b.textContent); listEl.style.display='none'});
  document.addEventListener('click',e=>{if(!listEl.contains(e.target)&&e.target!==input) listEl.style.display='none'});
}
bindSuggest(document.getElementById('preset'),document.getElementById('sugAp'),()=>AIRPORTS.length?AIRPORTS:FALLBACK_AP, r=>r[0]+' \u2014 '+(r[1]||r[5])+' \u00b7 '+(r[6]||r[4]||''), (code,label)=>{document.getElementById('preset').value=label;grabForm();loadLive(true)});
bindSuggest(document.getElementById('followAl'),document.getElementById('sugAl'),()=>ALINES, r=>r[2]+(r[1]?' ('+r[1]+')':'')+' \u00b7 '+r[0], (code,label)=>{document.getElementById('followAl').value=label;grabForm();applyFilters();renderWall()});
function letters(s){return (s||'').toUpperCase().replace(/[^A-Z]/g,'')}
function airlineOf(cs){const L=letters(cs);const hit=ALINES.find(a=>a[0]===L.slice(0,3));if(hit) return [hit[1]||hit[0].slice(0,2), hit[2], '#334155'];return ['', L.slice(0,3)||'ACFT', '#334155']}
function kindOf(a){const t=(a.t||'').toUpperCase(); const cs=(a.flight||'').toUpperCase(); const L=letters(cs);if(/^(R44|R66|B06|B407|H125|H135)/.test(t)) return 'heli';if(cs.startsWith('LV')&&!ALINES.some(x=>x[0]===L.slice(0,3))) return 'ga';if(ALINES.some(x=>x[0]===L.slice(0,3))||/^(A3|A2|B7|B38|E1|E19|CRJ)/.test(t)) return 'air';if(/^(LJ|C25|C56|GLF|GLEX)/.test(t)) return 'biz';return 'ga'}
function aliasCode(q){const s=(q||'').toUpperCase().replace(/[^A-Z0-9]/g,'');if(!s) return '';const hit=ALINES.find(a=>a[0]===s||a[1]===s||hay(a).replace(/[^a-z0-9]/g,'').includes(s.toLowerCase()));return hit?hit[0]:s.slice(0,3)}
function matchAirline(a,q){if(!q) return true;return letters(a.flight).startsWith(aliasCode(q))}
function fmtAlt(alt){if(alt==='ground'||alt==null||alt===0)return 'GND';const n=Number(alt);if(!n)return '-';return n>=1000?(n/1000).toFixed(1)+'k ft':Math.round(n)+' ft'}
function ktToMph(gs){return gs==null?'-':Math.round(gs*1.15078)+' mph'}
function hhmm(ts){if(!ts) return '--:--'; const d=new Date(ts);return String(d.getHours()).padStart(2,'0')+':'+String(d.getMinutes()).padStart(2,'0')}
function aptCoord(code){if(!code) return null;const a=APTINDEX[code];return a?[a[2],a[3]]:null}
function kmLL(a,b){return Math.hypot((a[0]-b[0])*111,(a[1]-b[1])*111*Math.cos(a[0]*Math.PI/180))}
const radar=document.getElementById('radar'), wall=document.getElementById('wall'), loadEl=document.getElementById('load');
let RAW=[],AC=[],running=false,looping=false,view='hybrid',theme='crt_amber',listStyle='fa';
let center=[-34.559,-58.416],includeGround=false,radiusKm=120,maxN=16,rotateS=7,house=false;
let beam=-Math.PI/2,lastSweep=0,lastRot=0,page=0,loading=false,followAl='',followFlt='';
let want={air:true,ga:false,biz:false,heli:false}; const POS={};
const fAir=document.getElementById('fAir'),fGa=document.getElementById('fGa'),fBiz=document.getElementById('fBiz'),fHeli=document.getElementById('fHeli');
function setLoad(msg,show){const st=document.getElementById('status');if(st)st.textContent=msg||'';if(show){loadEl.textContent=msg||'CARGANDO';loadEl.classList.add('on')}else loadEl.classList.remove('on')}
function enrich(a){
  const now=Date.now(); const key=(a.flight||'').replace(/\s+/g,'').toUpperCase(); const cached=ROUTECACHE[key];
  if(cached){a.origin=cached[0];a.dest=cached[1]}
  const gnd=a.alt==='ground'||a.alt==0||a.gs===0;
  const destLL=aptCoord(a.dest), origLL=aptCoord(a.origin), here=[a.lat,a.lon], gs=Number(a.gs)||0;
  if(gnd){a.status='EN TIERRA';a.depKind='PROG';a.arrKind='PROG';a.depAt=now+25*60000;a.arrAt=a.depAt+80*60000;a.late=false;return a}
  a.status='EN RUTA'; a.depKind='EST'; a.arrKind='EST'; a.late=false;
  if(destLL && gs>40){
    const rem=kmLL(here,destLL); const minRem=Math.max(8, Math.round(rem/(gs*1.852)*60));
    a.arrAt=now+minRem*60000;
    if(origLL){const tot=Math.max(rem+20, kmLL(origLL,destLL)); const done=Math.max(0.05, Math.min(0.95, 1-rem/tot)); a.depAt=now-Math.round(minRem*(done/(1-done)))*60000;}
    else a.depAt=now-40*60000;
  } else {a.depAt=null;a.arrAt=null}
  return a;
}
function norm(a){return {hex:a.hex,flight:(a.flight||a.r||a.hex||'').toString().trim(),r:a.r,t:a.t,lat:a.lat,lon:a.lon,alt:a.alt_baro!=null?a.alt_baro:(a.alt!=null?a.alt:a.alt_geom),gs:a.gs,track:a.track,dst:a.dst}}
function applyFilters(){AC=RAW.filter(a=>{if(!includeGround && (a.alt==='ground'||a.alt===0)) return false;if(a.dst!=null && a.dst*1.852>radiusKm) return false;if(!want[kindOf(a)]) return false;if(!matchAirline(a,followAl)) return false;if(followFlt && !(a.flight||'').toUpperCase().replace(/\s+/g,'').includes(followFlt.toUpperCase().replace(/\s+/g,''))) return false;return a.lat!=null}).sort((a,b)=>(a.dst||99)-(b.dst||99)).slice(0,maxN);AC.forEach(enrich)}
async function pullJson(url, ms){const ctrl=new AbortController();const t=setTimeout(()=>ctrl.abort(), ms||9000);try{const res=await fetch(url,{signal:ctrl.signal});if(!res.ok) throw new Error('bad '+res.status);return await res.json()}finally{clearTimeout(t)}}
async function fillRoutes(list){const pending=list.filter(a=>a.flight&&!ROUTECACHE[(a.flight||'').replace(/\s+/g,'').toUpperCase()]).slice(0,10);await Promise.all(pending.map(async a=>{const cs=(a.flight||'').replace(/\s+/g,'').toUpperCase();try{const d=await pullJson('https://vrs-standing-data.adsb.lol/routes/'+cs.slice(0,2)+'/'+cs+'.json',6000);const iata=(d._airport_codes_iata||'').split('-').filter(x=>x&&x.length<=4);if(iata.length>=2){ROUTECACHE[cs]=[iata[0],iata[iata.length-1]];a.origin=iata[0];a.dest=iata[iata.length-1]}}catch(e){}}))}
async function fetchSky(lat,lon,nm){
  const api='https://api.adsb.lol/v2/lat/'+lat+'/lon/'+lon+'/dist/'+nm;
  const urls=[api,'https://corsproxy.io/?'+encodeURIComponent(api),'https://api.allorigins.win/raw?url='+encodeURIComponent(api)];
  let lastErr;
  for(const u of urls){try{const data=await pullJson(u,10000);if(data&&Array.isArray(data.ac)) return {data,src:u===api?'vivo':'vivo proxy'}}catch(e){lastErr=e}}
  throw lastErr||new Error('sin cielo');
}
async function loadLive(showOverlay){
  if(loading) return; loading=true;
  const code=currentCode();
  if(showOverlay) setLoad('Cargando '+code+'...',true);
  RAW=[]; AC=[]; renderWall();
  const nm=Math.max(30,Math.round(radiusKm/1.852));
  try{
    const {data,src}=await fetchSky(center[0],center[1],nm);
    srcLabel=src; RAW=(data.ac||[]).map(norm); applyFilters(); renderWall();
    fillRoutes(AC).then(()=>{applyFilters();renderWall()});
    const now=performance.now(); AC.forEach(a=>{POS[a.hex]={lat:a.lat,lon:a.lon,t:now,gs:a.gs,track:a.track,alt:a.alt}}); lastSweep=now;
    const st=document.getElementById('status');
    if(st){
      if(!RAW.length) st.textContent='0 aeronaves en '+code+' (radio '+radiusKm+' km). No hay cobertura ADS-B o no hay trafico ahora.';
      else if(followAl&&!AC.length) st.textContent=RAW.length+' aviones en zona, 0 de '+followAl;
      else st.textContent=AC.length+' aeronaves \u00b7 '+code+' \u00b7 '+srcLabel;
    }
  }catch(e){
    RAW=[]; AC=[]; renderWall();
    const st=document.getElementById('status'); if(st) st.textContent='No pude leer el cielo de '+code+'. Reintenta en un momento.';
  } finally{loading=false; loadEl.classList.remove('on')}
}
function displayPos(a){const p=POS[a.hex];if(!p||a.alt==='ground'||!p.gs||p.track==null)return {lat:a.lat,lon:a.lon};const dt=Math.min(25,(performance.now()-p.t)/1000);const km=p.gs*1.852*dt/3600;const rad=p.track*Math.PI/180;return {lat:p.lat+Math.cos(rad)*km/111,lon:p.lon+Math.sin(rad)*km/(111*Math.cos(p.lat*Math.PI/180))}}
function colors(){if(theme==='crt_green')return {fg:'#7dff7a',bg:'#031105'};if(theme==='atc_dark')return {fg:'#7ec8e3',bg:'#07090c'};if(theme==='navy')return {fg:'#8ab4ff',bg:'#061018'};if(theme==='red')return {fg:'#ff6b4a',bg:'#120606'};if(theme==='ice')return {fg:'#d9f6ff',bg:'#081016'};return {fg:'#e8b86d',bg:'#0a0805'}}
function applyThemeCss(){const {fg,bg}=colors();document.documentElement.style.setProperty('--list',fg);document.documentElement.style.setProperty('--bg',bg);document.documentElement.style.setProperty('--card',bg);const b=document.querySelector('.bezel');if(b)b.style.background=bg}
function grabForm(){view=document.getElementById('mode').value;theme=document.getElementById('theme').value;listStyle=document.getElementById('listStyle').value;applyThemeCss();const ap=findAirport(document.getElementById('preset').value);center=[ap[2],ap[3]];followAl=document.getElementById('followAl').value.trim();followFlt=document.getElementById('followFlt').value.trim();house=document.getElementById('house').checked;includeGround=document.getElementById('ground').value==='1';radiusKm=Number(document.getElementById('radius').value)||120;maxN=Math.max(4,Math.min(24,Number(document.getElementById('max').value)||16));rotateS=Math.max(3,Math.min(30,Number(document.getElementById('rotate').value)||7));want={air:fAir.checked,ga:fGa.checked,biz:fBiz.checked,heli:fHeli.checked};radar.style.display=view==='wall'?'none':'block';wall.style.display=view==='radar'?'none':'flex';wall.classList.toggle('full',view==='wall')}
document.getElementById('geo').addEventListener('click',()=>{navigator.geolocation.getCurrentPosition(pos=>{home=[pos.coords.latitude,pos.coords.longitude];document.getElementById('preset').value='CASA';center=home;document.getElementById('house').checked=true;house=true;loadLive(true)})});
document.getElementById('demoHouse').addEventListener('click',()=>{document.getElementById('house').checked=true;document.getElementById('mode').value='radar';house=true;demoOver=true;if(!home)home=center.slice();grabForm();running=true;if(!looping)loop();updateOver()});
document.getElementById('f').addEventListener('submit',e=>{e.preventDefault();grabForm();page=0;loadLive(true);running=true;if(view!=='wall'&&!looping)loop()});
['radius','max','ground','mode','theme','listStyle','rotate','followFlt','fAir','fGa','fBiz','fHeli','house'].forEach(id=>{document.getElementById(id).addEventListener('change',()=>{grabForm();if(id==='radius') loadLive(true); else{applyFilters();renderWall()}running=true;if(view!=='wall'&&!looping)loop()})});
function stClass(a){return a.status==='EN TIERRA'?'st-gnd':'st-air'}
function logoHTML(a){const [iata,name,color]=airlineOf(a.flight);const fb='<div class="fb" style="background:'+color+'">'+(iata||name||'GA').slice(0,2)+'</div>';if(!iata)return fb;return '<img alt="'+iata+'" src="https://images.kiwi.com/airlines/64/'+iata+'.png" onerror="this.outerHTML=this.dataset.fb" data-fb=\''+fb+'\'>'}
function punct(a){return a.status==='EN TIERRA'?'EN TIERRA':'EN VUELO'}
function metLine(a){if(a.status==='EN TIERRA') return 'Preparandose para el despegue'; const bits=['Alt '+fmtAlt(a.alt), ktToMph(a.gs)]; if(a.arrAt){const m=Math.round((a.arrAt-Date.now())/60000); if(m>=0) bits.push('llega en '+m+' min')} return bits.join(' \u00b7 ')}
function cardHTML(a){const [,name]=airlineOf(a.flight); let pct=18; if(a.status==='EN TIERRA') pct=6; else if(a.depAt&&a.arrAt){const t=Date.now(); pct=Math.max(6,Math.min(94,((t-a.depAt)/(a.arrAt-a.depAt))*100))} return '<div class="fa-card"><div class="fa-row"><div class="logo">'+logoHTML(a)+'</div><div><div class="fa-top"><span class="fa-id">'+(a.flight||a.hex)+'</span><span class="fa-st '+stClass(a)+'">'+a.status+'</span></div><div class="fa-air">'+name+' \u00b7 '+(a.t||a.r||'-')+'</div></div></div><div class="fa-route"><span>'+(a.origin||currentCode())+'</span><span class="bar"><i style="width:'+pct+'%"></i></span><span>'+(a.dest||'---')+'</span></div><div class="fa-times"><div><span class="lbl">Despegue '+a.depKind+'</span><b>'+hhmm(a.depAt)+'</b></div><div><span class="lbl">Aterrizaje '+a.arrKind+'</span><b>'+hhmm(a.arrAt)+'</b></div></div><div class="fa-met">'+metLine(a)+'</div></div>'}
function clipCards(){if(listStyle==='carousel'||listStyle==='fids')return;const box=wall.getBoundingClientRect();[...wall.querySelectorAll('.fa-card')].forEach(c=>{const r=c.getBoundingClientRect();if(r.bottom>box.bottom+2||r.top<box.top-2)c.style.display='none'})}
function visibleList(){if(!AC.length)return [];if(listStyle==='carousel')return [AC[page%AC.length]];const h=wall.clientHeight||300;const n=listStyle==='fids'?Math.max(1,Math.floor((h-22)/30)):Math.max(1,Math.min(AC.length,12));const start=(page*n)%Math.max(1,AC.length);const out=[];for(let i=0;i<Math.min(n,AC.length);i++)out.push(AC[(start+i)%AC.length]);return out}
function overhead(){if(demoOver)return enrich({hex:'demo',flight:'AR1716',t:'A320',lat:center[0],lon:center[1],alt:4200,gs:210,track:180,origin:'AEP',dest:'COR'});if(!house||!home)return null;let best=null,bestD=9;AC.forEach(a=>{const d=kmLL([displayPos(a).lat,displayPos(a).lon],home);if(d<bestD){bestD=d;best=a}});return best}
function updateOver(){const el=document.getElementById('overCard');if(!el)return;const a=overhead();if(!a||view==='wall'){el.style.display='none';return}el.style.display='block';el.innerHTML=cardHTML(a)}
function renderWall(){applyThemeCss();updateOver();if(!AC.length){wall.innerHTML='<div class="fa-card" style="text-align:center">Sin vuelos en este radio.</div>';return}const list=visibleList();if(listStyle==='fids')wall.innerHTML='<table class="fids"><thead><tr><th></th><th>VUELO</th><th>RUTA</th><th>DEP</th><th>ARR</th><th>ESTADO</th></tr></thead><tbody>'+list.map(a=>'<tr><td><div class="logo" style="width:24px;height:24px">'+logoHTML(a)+'</div></td><td>'+(a.flight||'')+'</td><td>'+(a.origin||'')+' → '+(a.dest||'')+'</td><td>'+hhmm(a.depAt)+'</td><td>'+hhmm(a.arrAt)+'</td><td>'+punct(a)+'</td></tr>').join('')+'</tbody></table>';else wall.innerHTML=listStyle==='carousel'?'<div class="carousel-wrap">'+(list[0]?cardHTML(list[0]):'')+'</div>':list.map(cardHTML).join('');requestAnimationFrame(clipCards)}
function angDiff(a,b){let d=a-b;while(d>Math.PI)d-=Math.PI*2;while(d<-Math.PI)d+=Math.PI*2;return d}
function loop(){if(!running||view==='wall'){looping=false;return}looping=true;const ctx=radar.getContext('2d');const w=radar.width=radar.clientWidth||640;const h=radar.height=radar.clientHeight||480;const {fg,bg}=colors();ctx.fillStyle=bg;ctx.fillRect(0,0,w,h);const left=view==='hybrid'?0.54:1;const cx=w*left*0.5,cy=h*0.52;const R=Math.min(cx,cy)*0.88;ctx.strokeStyle=fg+'22';[0.25,0.5,0.75,1].forEach(r=>{ctx.beginPath();ctx.arc(cx,cy,R*r,0,Math.PI*2);ctx.stroke()});ctx.beginPath();ctx.moveTo(cx,cy-R);ctx.lineTo(cx,cy+R);ctx.moveTo(cx-R,cy);ctx.lineTo(cx+R,cy);ctx.stroke();beam+=0.012;if(beam>Math.PI*3)beam-=Math.PI*2;if(performance.now()-lastSweep>60000) loadLive(false);if(performance.now()-lastRot>rotateS*1000){lastRot=performance.now();page++;renderWall()}ctx.strokeStyle=fg;ctx.beginPath();ctx.moveTo(cx,cy);ctx.lineTo(cx+Math.cos(beam)*R,cy+Math.sin(beam)*R);ctx.stroke();ctx.fillStyle=fg;ctx.font='11px monospace';ctx.fillText(currentCode(),10,18);const span=Math.max(0.25,radiusKm/111);const labeled=new Set(visibleList().map(a=>a.hex));AC.forEach(a=>{const pos=displayPos(a);const x=cx+((pos.lon-center[1])/span)*R;const y=cy-((pos.lat-center[0])/span)*R;if(Math.abs(angDiff(Math.atan2(y-cy,x-cx),beam))<0.08) a.paint=1;a.paint=Math.max(0.28,(a.paint||0.28)-0.0025);ctx.globalAlpha=a.paint;ctx.fillStyle=fg;ctx.save();ctx.translate(x,y);ctx.rotate((a.track||0)*Math.PI/180);ctx.beginPath();ctx.moveTo(0,-6);ctx.lineTo(3.6,5);ctx.lineTo(-3.6,5);ctx.closePath();ctx.fill();ctx.restore();if(labeled.has(a.hex)) ctx.fillText((a.flight||'').slice(0,8),x+8,y-8);ctx.globalAlpha=1});if(demoOver){ctx.strokeStyle='#fff';ctx.beginPath();ctx.arc(cx,cy,18,0,Math.PI*2);ctx.stroke();ctx.fillStyle='#fff';ctx.beginPath();ctx.moveTo(cx,cy-10);ctx.lineTo(cx+7,cy+8);ctx.lineTo(cx-7,cy+8);ctx.closePath();ctx.fill();ctx.fillText('SOBRE CASA',cx+14,cy-8);updateOver()}requestAnimationFrame(loop)}
function parseCsv(text){const rows=[]; let row=[], cell='', q=false; for(let i=0;i<text.length;i++){const c=text[i]; if(q){ if(c==='"'&&text[i+1]==='"'){cell+='"';i++} else if(c==='"') q=false; else cell+=c; } else if(c==='"') q=true; else if(c===','){row.push(cell);cell=''} else if(c==='\n'){row.push(cell);rows.push(row);row=[];cell=''} else if(c!=='\r') cell+=c;} if(cell||row.length){row.push(cell);rows.push(row)} return rows}
async function boot(){setLoad('Cargando aeropuertos y aerolineas...',true);try{const [apTxt,alTxt]=await Promise.all([fetch('https://cdn.jsdelivr.net/gh/jpatokal/openflights@master/data/airports.dat').then(r=>r.text()),fetch('https://cdn.jsdelivr.net/gh/jpatokal/openflights@master/data/airlines.dat').then(r=>r.text())]);AIRPORTS=parseCsv(apTxt).map(r=>{const iata=(r[4]||'').trim();if(iata.length!==3||iata==='\\N') return null;return [iata, r[2]||r[1], +r[6], +r[7], r[3]||'', r[1]||'', r[3]||'', ''];}).filter(Boolean);AIRPORTS.forEach(a=>{if(!APTINDEX[a[0]]) APTINDEX[a[0]]=a});ALINES=parseCsv(alTxt).map(r=>{const icao=(r[4]||'').trim(), iata=(r[3]||'').trim(), active=r[7];if(icao.length!==3||icao==='\\N') return null;if(active==='N') return null;return [icao, iata==='\\N'||iata==='-'?'':iata, r[1]||icao, r[6]||'', r[5]||''];}).filter(Boolean);if(!ALINES.some(a=>a[0]==='TCV')) ALINES.push(['TCV','VR','Cabo Verde Airlines (TACV)','Cape Verde','CABOVERDE Cape Cabo Verde']);}catch(e){AIRPORTS=FALLBACK_AP; FALLBACK_AP.forEach(a=>APTINDEX[a[0]]=a)} grabForm(); await loadLive(true); running=true; if(view!=='wall') loop();}
boot();
setInterval(()=>{if(view!=='wall')return;if(performance.now()-lastRot>rotateS*1000){lastRot=performance.now();page++;renderWall()}},400);
window.addEventListener('resize',()=>renderWall());
