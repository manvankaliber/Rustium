import { chromium } from '@playwright/test';
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import http from 'node:http';
import { fileURLToPath } from 'node:url';
import { metrics, history, categories, aggregate, archiveFor, preferredRun, manifest, scopes, average, metricForRun, bridgeFor } from '../src/lib/model.js';

const site = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '..');
const scratch = path.resolve(site, '../.scratch/benchmarks-site');
const downloads = path.join(scratch,'exports'); fs.mkdirSync(downloads,{recursive:true});
const mime = { '.html':'text/html', '.js':'text/javascript', '.css':'text/css', '.json':'application/json', '.png':'image/png', '.mp4':'video/mp4', '.svg':'image/svg+xml' };
const server = http.createServer((req,res)=>{
  const url = decodeURIComponent(new URL(req.url,'http://localhost').pathname);
  const base = url.startsWith('/__artifacts/') ? scratch : path.join(site,'build');
  let file = path.resolve(base, '.'+(url.startsWith('/__artifacts/') ? url.slice('/__artifacts'.length) : url));
  if (!file.startsWith(base+path.sep) && file!==base) {res.writeHead(403).end();return;}
  if (fs.existsSync(file) && fs.statSync(file).isDirectory()) file=path.join(file,'index.html');
  if (!fs.existsSync(file)) {res.writeHead(404).end();return;}
  res.writeHead(200,{'content-type':mime[path.extname(file)]||'application/octet-stream'}); fs.createReadStream(file).pipe(res);
});
await new Promise(resolve=>server.listen(0,'127.0.0.1',resolve));
const origin = `http://127.0.0.1:${server.address().port}`;
const report = { engine: 'Installed Google Chrome headless on Windows; emulated CSS viewports, not physical devices', source_hashes: { html: manifest.canonical_sha256, data: manifest.benchmark_data_sha256, engine: manifest.engine_sha256 }, checks: [], artifacts: [], errors: [] };
let browser;
const record = (name,details={}) => { report.checks.push({name,result:'pass',...details}); console.log(name); };
try {
  browser = await chromium.launch({channel:'chrome',headless:true,args:['--disable-gpu']});
  report.browser_version = browser.version();
  const context = await browser.newContext({acceptDownloads:true,viewport:{width:1440,height:1000}});
  const page = await context.newPage();
  page.on('pageerror',error=>report.errors.push(error.message));
  const response=await page.goto(origin); assert.equal(response.status(),200);
  await page.locator('.overview .chart svg').waitFor();
  assert.equal(await page.locator('iframe').count(),0);
  assert.equal(Number(await page.locator('.overview').getAttribute('data-case-count')),aggregate.rows.length);
  assert(Math.abs(Number(await page.locator('.overview').getAttribute('data-speedup'))-aggregate.percent)<1e-9);
  assert.equal(await page.locator('.overview .configuration').isVisible(),false);
  assert.equal(await page.locator('.metric .exact-values[open]').count(),0);
  assert((await page.locator('.page-topline').textContent()).includes(history.find(entry=>entry.date)?.date || 'Measurement dates not recorded'));
  record('SSR data, aggregate, native content and hidden defaults');

  for (const width of [320,390,768,1440,1920]) {
    await page.setViewportSize({width,height:1000});
    await page.waitForTimeout(120);
    const overflow = await page.evaluate(()=>({viewport:innerWidth,scroll:document.documentElement.scrollWidth}));
    assert(overflow.scroll<=width,JSON.stringify(overflow));
    const card=page.locator('.overview');
    await card.locator('.config-button').click();
    assert.equal(await card.locator('.config-button').getAttribute('aria-expanded'),'true');
    assert.equal(await card.locator('.configuration').isVisible(),true);
    assert.equal(await card.locator('select:focus').count(),1);
    assert(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
    const controls = await card.locator('button,select').evaluateAll(nodes=>nodes.filter(n=>n.getClientRects().length).map(n=>({height:n.getBoundingClientRect().height,width:n.getBoundingClientRect().width})));
    assert(controls.every(c=>c.height>=44 && c.width>=44));
    await page.keyboard.press('Escape');
    assert.equal(await card.locator('.config-button').getAttribute('aria-expanded'),'false');
    assert.equal(await card.locator('.config-button:focus').count(),1);
    await page.evaluate(()=>scrollTo(0,0));
    const artifact=path.join(scratch,`dashboard-${width}.png`); await page.screenshot({path:artifact,fullPage:false}); report.artifacts.push(artifact);
    record(`Responsive ${width}px, controls, Escape and focus return`);
  }
  await page.setViewportSize({width:1440,height:1000});
  const overviewCard=page.locator('.overview');
  await overviewCard.locator('.config-button').click();
  for (const [scope] of scopes) for (const method of ['geometric','arithmetic']) {
    await overviewCard.locator('[data-option=scope]').selectOption(scope);
    await overviewCard.locator('[data-option=average]').selectOption(method);
    assert(Math.abs(Number(await overviewCard.getAttribute('data-speedup'))-average(scope,method).percent)<1e-9);
  }
  await overviewCard.locator('[data-option=scope]').selectOption('all');await overviewCard.locator('[data-option=average]').selectOption('geometric');
  const overviewDl=page.waitForEvent('download');await overviewCard.getByRole('button',{name:/as PNG$/}).click();const overviewPng=await overviewDl;const overviewPath=path.join(downloads,'overview.png');await overviewPng.saveAs(overviewPath);report.artifacts.push(overviewPath);
  await overviewCard.locator('.configuration button').click();record('Aggregate scope, method and PNG export');
  for (const category of categories) {
    await page.locator('.desktop-nav button').filter({hasText:category}).click();
    const ids=await page.locator('.feed .metric').evaluateAll(nodes=>nodes.map(n=>n.dataset.metric));
    assert(ids.length>0); assert(ids.every(id=>history.some(e=>e.metric.id===id && e.category===category)));
  }
  await page.locator('.desktop-nav button').filter({hasText:'Overview'}).click();
  record('Subsystem navigation derives from recorded metrics');

  const search=page.locator('input[type=search]');
  await search.fill('___no_measurement___'); await page.locator('.empty-state').waitFor();
  await search.fill('');
  await page.locator('.history-toggle input').check();
  const order=await page.locator('.feed .metric').evaluateAll(nodes=>nodes.map(n=>`${n.dataset.metric}:${n.dataset.run}`));
  assert.deepEqual(order,history.slice(0,order.length).map(e=>e.id));
  await page.locator('.history-toggle input').uncheck();
  const initialCount=await page.locator('.feed .metric').count();
  if (initialCount < metrics.length) {
    await page.getByRole('button',{name:'Load more benchmarks'}).click();
    assert(await page.locator('.feed .metric').count()>initialCount);
  }
  record('Search, empty state and recorded-date history ordering');
  await search.fill('');
  let selections=0;
  for (const metric of metrics) {
    await search.fill(metric.title);
    const card=page.locator(`.feed [data-metric="${metric.id}"]`);
    await card.scrollIntoViewIfNeeded(); await card.locator('.chart svg').waitFor();
    assert.equal(await card.getAttribute('data-run'),preferredRun(metric));
    await card.locator('.config-button').click();
    for (const [run] of metric.runs || [['','']]) {
      if (metric.runs) await card.locator('[data-option=run]').selectOption(run);
      const current=metricForRun(metric,run), bridge=bridgeFor(metric,run);
      assert.equal(await card.locator('.candidate').count(),current.include_in_average===false?1:0);
      assert.equal((await card.locator('.bridge summary').textContent()).replace('ⓘ','').trim(),bridge.label);
      assert((await card.locator('.bridge > div').textContent()).includes(bridge.context));
      assert((await card.locator('.chart svg').textContent()).includes(current.production_status==='deferred'?'Candidate only':'Rustium'));
      const packs=metric.source==='lookup' ? [...new Set(metric.rows.map(r=>r.pack))] : [''];
      for (const pack of packs) {
        if (pack) await card.locator('[data-option=pack]').selectOption(pack);
        const expected=metric.rows.filter(r=>(!run || r.run===run)&&(!pack||r.pack===pack));
        await card.locator('.exact-values > summary').click();
        const values=await card.locator('tbody tr').evaluateAll(nodes=>nodes.map(n=>[...n.querySelectorAll('td')].slice(0,2).map(td=>Number(td.textContent))));
        assert.deepEqual(values,expected.map(r=>[r.baseline_median_ns,r.rust_median_ns]));
        const href=await card.locator('.metric-evidence').getAttribute('href');assert.equal(href,`./evidence/${archiveFor(metric,run)}`);
        assert.equal((await page.request.get(new URL(href,origin).href)).status(),200);
        await card.locator('.exact-values > summary').click();
        selections++;
      }
    }
    await card.locator('[data-option=input]').selectOption({index:1});
    assert.equal(Number(await card.getAttribute('data-case-count')),1);
    for (const view of ['saved','time','ratio','delta']) { await card.locator('[data-option=view]').selectOption(view); await card.locator('.chart svg').waitFor(); }
    await card.locator('[data-option=layout]').selectOption('vertical');
    for(const key of ['legend','details']) await card.locator(`[data-option=${key}]`).selectOption('show');
    assert((await card.locator('.chart svg').textContent()).includes('C++'));
    await card.locator('[data-option=input]').selectOption('all');
    for (const width of [320,1440]) {
      await page.setViewportSize({width,height:1000});
      await page.waitForTimeout(30);
      for (const layout of ['horizontal','vertical']) {
        await card.locator('[data-option=layout]').selectOption(layout);
        const bounds=await card.locator('.chart svg').evaluate(svg=>{
          const errors=[],box=svg.viewBox.baseVal;
          for(const node of svg.querySelectorAll('text')) {
            const b=node.getBBox();
            if(b.x < -1 || b.y < -1 || b.x+b.width > box.width+1 || b.y+b.height > box.height+1) errors.push({text:node.textContent,x:b.x,y:b.y,width:b.width,height:b.height,viewBox:[box.width,box.height]});
          }
          const chart=svg.closest('.chart').getBoundingClientRect(),actions=svg.closest('.metric').querySelector('.actions').getBoundingClientRect();
          return {errors,svgBottom:svg.getBoundingClientRect().bottom,chartBottom:chart.bottom,actionsTop:actions.top,overflow:document.documentElement.scrollWidth>innerWidth};
        });
        assert.deepEqual(bounds.errors,[],JSON.stringify({metric:metric.id,width,layout,...bounds}));
        assert(bounds.svgBottom<=bounds.chartBottom+1 && bounds.chartBottom<=bounds.actionsTop+1 && !bounds.overflow,JSON.stringify(bounds));
      }
    }
    await card.locator('.configuration button').click();
  }
  record('All metrics: historical runs, packs, raw-median tables, evidence links, input, comparison, orientation, legend and notes', {selections, metric_groups:metrics.length});

  await search.fill('Load & validate a resource pack');
  const card=page.locator('[data-metric=load]');
  await card.scrollIntoViewIfNeeded();
  // Filtering remounts cards, so verify defaults and export the full case set.
  const dl=page.waitForEvent('download'); await card.getByRole('button',{name:/as PNG$/}).click();
  const png=await dl;const pngPath=path.join(downloads,'load-default.png');await png.saveAs(pngPath);report.artifacts.push(pngPath);
  const pngInfo=await page.evaluate(async url=>{const i=new Image();i.src=url;await i.decode();const c=document.createElement('canvas');c.width=i.width;c.height=i.height;const ctx=c.getContext('2d');ctx.drawImage(i,0,0);return {width:i.width,height:i.height,corner:[...ctx.getImageData(0,0,1,1).data]};},`${origin}/__artifacts/exports/load-default.png`);
  assert.deepEqual(pngInfo,{width:1600,height:1600,corner:[255,255,255,255]});
  record('Single-click white 1600 × 1600 PNG', {file:pngPath});
  // Selected options must change the share composition, independently of viewport.
  await card.locator('.config-button').click();
  await card.locator('[data-option=view]').selectOption('saved');await card.locator('[data-option=layout]').selectOption('vertical');await card.locator('[data-option=input]').selectOption('empty');
  const customDl=page.waitForEvent('download');await card.getByRole('button',{name:/as PNG$/}).click();const custom=await customDl;const customPath=path.join(downloads,'load-selected.png');await custom.saveAs(customPath);report.artifacts.push(customPath);assert(!fs.readFileSync(customPath).equals(fs.readFileSync(pngPath)));
  await card.locator('[data-option=duration]').selectOption('8');
  const supported=await page.evaluate(async()=>typeof VideoEncoder!=='undefined' && (await VideoEncoder.isConfigSupported({codec:'avc1.420028',width:1080,height:1080,bitrate:6000000,framerate:30,latencyMode:'realtime',avc:{format:'avc'}})).supported);
  if(supported) {
    await page.evaluate(()=>{window.__encodedFrames=0;const encode=VideoEncoder.prototype.encode;VideoEncoder.prototype.encode=function(...args){window.__encodedFrames++;return encode.apply(this,args);};});
    for(const duration of ['8','61']) {
      await card.locator('[data-option=duration]').selectOption(duration);
      const begin=Date.now(),videoDl=page.waitForEvent('download');await card.getByRole('button',{name:/as MP4$/}).click();const video=await videoDl;const videoPath=path.join(downloads,`load-${duration}s.mp4`);await video.saveAs(videoPath);report.artifacts.push(videoPath);
      const playback=await page.evaluate(async url=>{const v=document.createElement('video');v.preload='metadata';v.src=url;await new Promise((resolve,reject)=>{v.onloadedmetadata=resolve;v.onerror=reject;});const result={width:v.videoWidth,height:v.videoHeight,duration:v.duration,loop:v.loop};v.remove();return result;},`${origin}/__artifacts/exports/load-${duration}s.mp4`);
      assert.deepEqual(playback,{width:1080,height:1080,duration:Number(duration),loop:false});
      const buffer=fs.readFileSync(videoPath); const stsz=buffer.indexOf(Buffer.from('stsz')); assert(stsz>0);const samples=buffer.readUInt32BE(stsz+12);assert.equal(samples,70);
      assert.equal(await page.evaluate(()=>window.__encodedFrames),70,'61s must reuse the cached 8s encoding');
      record(`Direct H.264 ${duration}s export${duration==='61'?' with reused encoding':''}`,{wall_ms:Date.now()-begin,samples,playback,file:videoPath});
    }
    // Start uncached work and cancel before the queued encoder finishes.
    await card.locator('[data-option=input]').selectOption('all');
    await card.getByRole('button',{name:/as MP4$/}).click();await card.getByRole('button',{name:'Cancel export'}).click();
    await card.locator('.status').filter({hasText:'cancelled'}).waitFor();assert.equal(await card.getByRole('button',{name:/as MP4$/}).isEnabled(),true);record('MP4 cancellation unlocks controls');
    await page.emulateMedia({reducedMotion:'reduce'});
    const reducedDl=page.waitForEvent('download');await card.getByRole('button',{name:/as MP4$/}).click();const reduced=await reducedDl;const reducedPath=path.join(downloads,'load-reduced-motion.mp4');await reduced.saveAs(reducedPath);report.artifacts.push(reducedPath);const bytes=fs.readFileSync(reducedPath);assert.equal(bytes.readUInt32BE(bytes.indexOf(Buffer.from('stsz'))+12),1);record('Reduced-motion MP4 encodes one static sample');
    await page.emulateMedia({reducedMotion:'no-preference'});
  } else report.checks.push({name:'H.264 exports',result:'unsupported',reason:'Installed browser did not advertise AVC encoding'});
  await page.evaluate(()=>{window.VideoEncoder=undefined;});await card.getByRole('button',{name:/as MP4$/}).click();await card.locator('.status').filter({hasText:'unavailable'}).waitFor();assert.equal(await card.getByRole('button',{name:/as PNG$/}).isEnabled(),true);record('Unsupported WebCodecs gives a recoverable message');

  await card.locator('.configuration button').click();
  // Native controls are exercised with synthesized touch, beyond CSS resizing.
  const touchContext=await browser.newContext({viewport:{width:390,height:844},isMobile:true,hasTouch:true});
  const touch=await touchContext.newPage();await touch.goto(origin);await touch.locator('.mobile-nav select').selectOption(categories[0]);const touchCard=touch.locator('.feed .metric').first();await touchCard.locator('.config-button').tap();await touchCard.locator('[data-option=layout]').selectOption('vertical');assert.equal(await touchCard.locator('.configuration').isVisible(),true);assert(await touch.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));await touchCard.getByRole('button',{name:'Done'}).tap();assert.equal(await touchCard.locator('.config-button').getAttribute('aria-expanded'),'false');
  const touchPath=path.join(scratch,'touch-chart-390.png');await touchCard.screenshot({path:touchPath});report.artifacts.push(touchPath);await touchContext.close();record('Synthesized Chromium touch: category, configuration, select and Done');
  const imagePage=await context.newPage();await imagePage.setViewportSize({width:720,height:720});await imagePage.goto(`${origin}/__artifacts/exports/load-default.png`);await imagePage.evaluate(()=>{document.body.style.margin='0';document.querySelector('img').style.cssText='width:720px;height:720px;display:block;margin:0;';});const reducedImage=path.join(scratch,'export-720.png');await imagePage.screenshot({path:reducedImage});report.artifacts.push(reducedImage);await imagePage.close();
  assert.deepEqual(report.errors,[]);record('No browser runtime errors');
  report.result='pass';
} catch(error) {report.result='fail';report.failure=error.stack;console.error(error);process.exitCode=1;}
finally {if(browser) await browser.close();await new Promise(resolve=>server.close(resolve));fs.writeFileSync(path.join(scratch,'smoke-validation.json'),JSON.stringify(report,null,2));}
