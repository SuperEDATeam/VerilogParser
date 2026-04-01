const express = require('express');
const multer  = require('multer');
const { exec } = require('child_process');
const cors   = require('cors');
const path   = require('path');
const fs     = require('fs');

const app  = express();
const PORT = 3001;

app.use(cors());

// ---------- Directories ----------

// backend/ is the CWD when started by the 'web' command.
// output/ and input/ live one level up in main/.
const projectRoot = path.join(__dirname, '..');
const outputDir   = path.join(projectRoot, 'output');
const uploadsDir  = path.join(__dirname, 'uploads');

if (!fs.existsSync(uploadsDir)) fs.mkdirSync(uploadsDir, { recursive: true });
if (!fs.existsSync(outputDir))  fs.mkdirSync(outputDir,  { recursive: true });

// ---------- Locate Main.exe ----------
// Search several candidate paths so the server works regardless of how the
// project was built or where Main.exe was copied.
function findMainExe() {
  const candidates = [
    path.join(__dirname, 'Main.exe'),                           // backend/Main.exe  (manual copy)
    path.join(projectRoot, 'x64', 'Debug',   'Main.exe'),      // VS Debug build
    path.join(projectRoot, 'x64', 'Release', 'Main.exe'),      // VS Release build
    path.join(projectRoot, 'Main.exe'),                         // project root
  ];
  for (const p of candidates) {
    if (fs.existsSync(p)) return p;
  }
  return null;
}

// ---------- File upload ----------

const storage = multer.diskStorage({
  destination: (req, file, cb) => cb(null, uploadsDir),
  filename:    (req, file, cb) => cb(null, Date.now() + '-' + file.originalname),
});
const upload = multer({ storage });

// Serve output files (dot, png, il, blif) as static assets
app.use('/output', express.static(outputDir));

// ---------- POST /process ----------

app.post('/process', upload.single('verilogFile'), (req, res) => {
  if (!req.file) {
    return res.status(400).json({ error: 'No file uploaded' });
  }

  const mainExe = findMainExe();
  if (!mainExe) {
    fs.unlink(req.file.path, () => {});
    return res.status(500).json({
      error: 'Main.exe not found. Build the C++ project first (Debug|x64).',
    });
  }

  const inputFile  = req.file.path;
  const dotFile    = path.join(outputDir, 'show1.dot');
  const pngFile    = path.join(outputDir, 'show1.png');
  const rtlilFile  = path.join(outputDir, 'output.il');

  // Invoke Main.exe in batch mode using the new CLI:
  //   Main.exe -v <input.v> -o <output.il> -d <show1.dot>
  const command = `"${mainExe}" -v "${inputFile}" -o "${rtlilFile}" -d "${dotFile}"`;
  console.log('Executing:', command);

  exec(command, { cwd: projectRoot }, (err, stdout, stderr) => {
    // Always clean up the upload
    fs.unlink(inputFile, () => {});

    if (err) {
      console.error('Execution error:', err.message);
      return res.status(500).json({
        error: 'Main.exe failed',
        details: err.message,
        stderr,
        stdout,
      });
    }

    // Convert DOT -> PNG via Graphviz (best-effort; Graphviz must be in PATH)
    const dotCmd = `dot -Tpng "${dotFile}" -o "${pngFile}"`;
    exec(dotCmd, () => {
      const imageUrl = fs.existsSync(pngFile)
        ? `http://localhost:${PORT}/output/show1.png`
        : null;

      const rtlilContent = fs.existsSync(rtlilFile)
        ? fs.readFileSync(rtlilFile, 'utf8')
        : null;

      res.json({
        result:       stdout || '(no stdout)',
        stderr:       stderr || undefined,
        imageUrl,
        rtlilContent,
        rtlilUrl: rtlilContent ? `http://localhost:${PORT}/output/output.il` : null,
      });
    });
  });
});

// ---------- Error handler ----------

app.use((err, req, res, next) => {
  console.error(err.stack);
  res.status(500).json({ error: 'Internal server error', details: err.message });
});

// ---------- Start ----------

app.listen(PORT, () => {
  console.log(`VerilogCompiler Backend running at http://localhost:${PORT}`);
  console.log(`Project root : ${projectRoot}`);
  console.log(`Output dir   : ${outputDir}`);
  console.log(`Main.exe     : ${findMainExe() || '*** NOT FOUND ***'}`);
});
