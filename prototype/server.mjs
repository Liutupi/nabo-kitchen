import http from 'node:http';
import {readFile,readdir,stat} from 'node:fs/promises';
import path from 'node:path';
import {fileURLToPath} from 'node:url';
const root=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'..');
const mime={'.html':'text/html; charset=utf-8','.mjs':'text/javascript; charset=utf-8','.js':'text/javascript; charset=utf-8','.css':'text/css; charset=utf-8','.json':'application/json; charset=utf-8','.png':'image/png','.svg':'image/svg+xml','.woff2':'font/woff2','.mp4':'video/mp4'};
const server=http.createServer(async(req,res)=>{
  try{
    const url=new URL(req.url,'http://localhost');
    if(url.pathname==='/api/recipes'){
      const dir=path.join(root,'data/recipes/curated');const files=(await readdir(dir)).filter(x=>x.endsWith('.json')).sort();
      const recipes=await Promise.all(files.map(async x=>JSON.parse(await readFile(path.join(dir,x),'utf8'))));
      res.writeHead(200,{'Content-Type':mime['.json'],'Cache-Control':'no-store'});res.end(JSON.stringify(recipes));return;
    }
    const pathname=decodeURIComponent(url.pathname==='/'?'/prototype/index.html':url.pathname);
    if(!['/prototype/','/game-art/kitchen/','/data/'].some(p=>pathname.startsWith(p))){res.writeHead(403);res.end('Forbidden');return;}
    const file=path.resolve(root,'.'+pathname);
    if(!['prototype','game-art/kitchen','data'].some(p=>file.startsWith(path.resolve(root,p)+path.sep))){res.writeHead(403);res.end('Forbidden');return;}
    if(!(await stat(file)).isFile())throw Error('Not a file');
    const content=await readFile(file);res.writeHead(200,{'Content-Type':mime[path.extname(file)]||'application/octet-stream','Cache-Control':'no-cache'});res.end(content);
  }catch{res.writeHead(404);res.end('Not found');}
});
const port=Number(process.env.PORT||process.argv[2]||4173);
server.listen(port,'127.0.0.1',()=>console.log(`NABO kitchen: http://127.0.0.1:${server.address().port}/`));
for(const signal of ['SIGTERM','SIGINT'])process.on(signal,()=>server.close(()=>process.exit(0)));
