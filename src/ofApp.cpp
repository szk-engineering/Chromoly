#include "ofApp.h"

ofDirectory targetDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/import");
ofDirectory exportDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/export");

void ofApp::setup()
{
  ofSetWindowTitle("Chromoly (NINA RICCI 2016 Version 20161218)");
  ofSetVerticalSync(true);
  ofEnableAlphaBlending();
  ofSetFrameRate(previewFramerate);
  ofLogToFile(exportDirectory.getAbsolutePath() + "/Chromoly_" + ofGetTimestampString("%Y%m%d") + ".log", true);

  if (settings.loadFile(SETTINGS_XML)) {
    logWithTimestamp("SETTING_XML has been loaded.");
    previewFramerate    = settings.getValue("previewFramerate", previewFramerate);
    chromaKey.keyColor  = ofColor::fromHex(settings.getValue("keyColor", 0x00ff00));
    chromaKey.threshold = settings.getValue("threshold", 0.1);
    webCaptureFrame     = settings.getValue("webCaptureFrame", webCaptureFrame);
    webOverlayScale     = settings.getValue("webOverlayScale", webOverlayScale);
    webOverlayX         = settings.getValue("webOverlayX", webOverlayX);
    webOverlayY         = settings.getValue("webOverlayY", webOverlayY);
    snsOverlayScale     = settings.getValue("snsOverlayScale", snsOverlayScale);
    snsOverlayX         = settings.getValue("snsOverlayX", snsOverlayX);
    snsOverlayY         = settings.getValue("snsOverlayY", snsOverlayY);
    cropSize            = settings.getValue("cropSize", cropSize);
  } else {
    logWithTimestamp("SETTING_XML couldn't been loaded.");
    chromaKey.keyColor  = ofColor::green;
    chromaKey.threshold = 0.1;
  }

  gui = new ofxDatGui(0, 0);
  gui->setTheme(new ofxDatGuiCustomFontSize);
  textVisitorNumber = gui->addTextInput("Visitor Number", ofToString(ofApp::getNewVisitorNumber()));
  textVisitorNumber->setInputType(ofxDatGuiInputType::NUMERIC);
  textVisitorNumber->onTextInputEvent(this, &ofApp::onTextVisitorNumberEvent);
  gui->addFRM();
  sliderPreviewFramerate = gui->addSlider("Preview fps", 0.0, 60.0);
  sliderPreviewFramerate->setPrecision(0);
  sliderPreviewFramerate->bind(previewFramerate);
  sliderPreviewFramerate->onSliderEvent(this, &ofApp::onSliderPreviewFramerateEvent);
  buttonReload = gui->addButton("[R]eload");
  buttonReload->onButtonEvent(this, &ofApp::onButtonReloadEvent);
  buttonReload->setLabelAlignment(ofxDatGuiAlignment::CENTER);
  sliderCurrentFrame = gui->addSlider("Current Frame", 0.0, 30.0, 0.0);
  sliderCurrentFrame->setPrecision(0);
  sliderCurrentFrame->bind(currentFrame);
  folderChromakey = gui->addFolder("Chroma Key");
  colorPicker     = folderChromakey->addColorPicker(" - Key Color");
  colorPicker->onColorPickerEvent(this, &ofApp::onColorPickerEvent);
  colorPicker->setColor(chromaKey.keyColor);
  sliderThreshold = folderChromakey->addSlider(" - Threshold", 0, 1.0, chromaKey.threshold);
  sliderThreshold->setPrecision(3);
  sliderThreshold->bind(chromaKey.threshold);
  folderChromakey->expand();
  folderWebOverlay      = gui->addFolder("For Web:");
  sliderWebCaptureFrame = folderWebOverlay->addSlider(" - Capture Frame", 0.0, 30.0);
  sliderWebCaptureFrame->setPrecision(0);
  sliderWebCaptureFrame->bind(webCaptureFrame);
  sliderWebOverlayScale = folderWebOverlay->addSlider(" - Scale", 0.0, 1.0);
  sliderWebOverlayScale->bind(webOverlayScale);
  padWebPosition = folderWebOverlay->add2dPad(" - Position");
  padWebPosition->setBounds(ofRectangle(0, 0, WEB_WIDTH, WEB_HEIGHT));
  padWebPosition->setPoint(ofPoint(webOverlayX, webOverlayY));
  padWebPosition->on2dPadEvent(this, &ofApp::onPadWebPosition);
  folderSnsOverlay      = gui->addFolder("For SNS:");
  sliderSnsOverlayScale = folderSnsOverlay->addSlider(" - Scale", 0.0, 1.0);
  sliderSnsOverlayScale->bind(snsOverlayScale);
  padSnsPosition = folderSnsOverlay->add2dPad(" - Position");
  padSnsPosition->setBounds(ofRectangle(0, 0, SNS_WIDTH, SNS_HEIGHT));
  padSnsPosition->setPoint(ofPoint(snsOverlayX, snsOverlayY));
  padSnsPosition->on2dPadEvent(this, &ofApp::onPadSnsPosition);
  buttonExport = gui->addButton("[E]xport");
  buttonExport->onButtonEvent(this, &ofApp::onButtonExportEvent);
  buttonExport->setLabelAlignment(ofxDatGuiAlignment::CENTER);
  gui->addBreak()->setHeight(10.0f);
  sliderCropSize = gui->addSlider("Crop Size", 0.0, 1080.0, cropSize);
  sliderCropSize->setPrecision(0);
  sliderCropSize->bind(cropSize);
  buttonUpload = gui->addButton("[U]pload All");
  buttonUpload->onButtonEvent(this, &ofApp::onButtonUploadEvent);
  buttonUpload->setLabelAlignment(ofxDatGuiAlignment::CENTER);
  buttonPrintQR = gui->addButton("[P]rint QR code");
  buttonPrintQR->onButtonEvent(this, &ofApp::onButtonPrintQREvent);
  buttonPrintQR->setLabelAlignment(ofxDatGuiAlignment::CENTER);
  windowResized(2106, 1080);

  fbo_android.allocate(ANDROID_WIDTH, ANDROID_HEIGHT);
  fbo_sns.allocate(SNS_WIDTH, SNS_HEIGHT);
  fbo_web.allocate(WEB_WIDTH, WEB_HEIGHT);

  importAndroidBackgrounds();
  importSnsBackgrounds();
  importWebBackground();
}

//  +-----+-----+---------+
//  | RAW | WEB |         |
//  +-----+-----+ ANDROID +
//  | GUI | SNS |         |
//  +-----+-----+---------+
void ofApp::update()
{
  // LEFT TOP:      RAW

  // LEFT BOTTOM:   GUI

  // MIDDLE TOP:    WEB
  fbo_web.begin();
  ofClear(0);
  if (isWebBackgroundLoaded) {
    webBackgroundImage.draw(0, 0);
  }
  if (isTargetLoaded) {
    chromaKey.begin();
    targetImages[webCaptureFrame].setAnchorPercent(0.5, 0.5);
    targetImages[webCaptureFrame].draw(webOverlayX, webOverlayY, WEB_WIDTH * webOverlayScale, WEB_HEIGHT * webOverlayScale);
    targetImages[webCaptureFrame].resetAnchor();
    chromaKey.end();
  }
  fbo_web.end();

  // MIDDLE BOTTOM: SNS
  fbo_sns.begin();
  ofClear(0);
  if (isSnsBackgroundLoaded) {
    snsBackgroundImage.draw(0, 0);
  }
  if (isTargetLoaded) {
    chromaKey.begin();
    targetImages[currentFrame].setAnchorPercent(0.5, 0.5);
    targetImages[currentFrame].draw(snsOverlayX, snsOverlayY, SNS_WIDTH * snsOverlayScale, SNS_HEIGHT * snsOverlayScale);
    targetImages[currentFrame].resetAnchor();
    chromaKey.end();
  }
  fbo_sns.end();

  // RIGHT:         ANDROID
  fbo_android.begin();
  ofSetColor(0);
  ofDrawRectangle(0, 0, fbo_android.getWidth(), fbo_android.getHeight());
  ofSetColor(255);

  // // +------------+
  // // |   TARGET   |
  // // +------------+
  // // | BACKGROUND |
  // // +------------+
  // if (isTargetLoaded) {
  //   chromaKey.begin();
  //   targetImages[currentFrame].draw(ANDROID_WIDTH / 2 - TARGET_WIDTH / 2, 651 - TARGET_HEIGHT);
  //   chromaKey.end();
  // }
  // if (isAndroidBackgroundLoaded) {
  //   androidBackgroundImage.draw(0, 651 + 93);
  // }

  // +------------+
  // | BACKGROUND |
  // +------------+
  // |   TARGET   |
  // +------------+
  if (isAndroidBackgroundLoaded) {
    androidBackgroundImage.draw(0, 0);
  }
  if (isTargetLoaded) {
    ofImage flipped;
    flipped.clone(targetImages[currentFrame]);
    flipped.mirror(true, false);
    chromaKey.begin();
    flipped.draw(ANDROID_WIDTH / 2 - TARGET_WIDTH / 2, 651 + 93, TARGET_WIDTH, TARGET_HEIGHT);
    chromaKey.end();
  }

  fbo_android.end();
}

void ofApp::draw()
{
  // LEFT TOP:      RAW
  if (isTargetLoaded) {
    targetImages[currentFrame].draw(0, 0, ofGetWidth() * leftPaneRatio, ofGetWidth() * leftPaneRatio);
  }

  // LEFT BOTTOM:   GUI

  // MIDDLE TOP:    WEB
  webCheckerImage.draw(ofGetWidth() * leftPaneRatio, 0, ofGetWidth() * middlePaneRatio, ofGetWidth() * middlePaneRatio);
  fbo_web.draw(ofGetWidth() * leftPaneRatio, 0, ofGetWidth() * middlePaneRatio, ofGetWidth() * middlePaneRatio);

  // MIDDLE BOTTOM: SNS
  fbo_sns.draw(ofGetWidth() * leftPaneRatio, ofGetHeight() / 2, ofGetWidth() * middlePaneRatio, ofGetWidth() * middlePaneRatio);
  if (isExporting) {
    ofApp::exportForSns();
  }

  // RIGHT:         ANDROID
  fbo_android.draw(ofGetWidth() * (1 - rightPaneRatio), 0, ofGetWidth() * rightPaneRatio, ofGetHeight());
  if (isExporting) {
    ofApp::exportForAndroid();
  }

  if (isTargetLoaded && currentFrame < targetImages.size() - 1) {
    currentFrame++;
  } else {
    currentFrame = 0;
    if (isExporting) {
      ofApp::exportFinish();
    }
  }
}

void ofApp::keyPressed(int key)
{
  switch (key) {
    case 'x':
      // For Debug!
      break;
    case 'i':
    case 'l':
    case 't':
    case 'r':
      importTargets();
      break;
    case 'w':
      importWebBackground();
      break;
    case 's':
      importSnsBackgrounds();
      break;
    case 'a':
      importAndroidBackgrounds();
      break;
    case 'b':
      importWebBackground();
      importSnsBackgrounds();
      importAndroidBackgrounds();
      break;
    case 'e':
      exportStart();
      break;
    case 'u':
      uploadAll();
      break;
    case 'p':
      printLastQRcode();
      break;
    case OF_KEY_UP:
      chromaKey.threshold += 0.005;
      break;
    case OF_KEY_DOWN:
      chromaKey.threshold -= 0.005;
      break;
    case OF_KEY_RIGHT:
      webCaptureFrame++;
      if (targetImages.size() - 1 < webCaptureFrame) {
        webCaptureFrame = 0;
      }
      break;
    case OF_KEY_LEFT:
      webCaptureFrame--;
      if (webCaptureFrame < 0) {
        webCaptureFrame = targetImages.size() - 1;
      }
      break;
    default:
      break;
  }
}

void ofApp::keyReleased(int key)
{
}

void ofApp::mouseDragged(int x, int y, int button)
{
}

void ofApp::mousePressed(int x, int y, int button)
{
}

void ofApp::mouseReleased(int x, int y, int button)
{
  if (button == 2) {
    // Right Click
    if (isTargetLoaded && x < ofGetWidth() * leftPaneRatio && y < ofGetWidth() * leftPaneRatio) {
      // Inside RAW Area
      int actualX = x * (1.0 * desirableWidth / ofGetWidth());
      int actualY = y * (1.0 * desirableHeight / ofGetHeight());
      chromaKey.keyColor = targetImages[webCaptureFrame].getColor(actualX * (cropSize / TARGET_WIDTH), actualY * (cropSize / TARGET_HEIGHT));
      colorPicker->setColor(chromaKey.keyColor);
    }
  }
}

void ofApp::windowResized(int w, int h)
{
  ofSetWindowShape(ofGetHeight() * windowAspectRatio, ofGetHeight());
  gui->setPosition(0, ofGetHeight() / 2);
  gui->setWidth(ofGetWidth() * leftPaneRatio);
}

void ofApp::dragEvent(ofDragInfo dragInfo)
{
}

void ofApp::gotMessage(ofMessage msg)
{
  say(msg.message);
}

void ofApp::exit()
{
  settings.setValue("previewFramerate", previewFramerate);
  settings.setValue("keyColor", chromaKey.keyColor.getHex());
  settings.setValue("threshold", chromaKey.threshold);
  settings.setValue("webCaptureFrame", webCaptureFrame);
  settings.setValue("webOverlayScale", webOverlayScale);
  settings.setValue("webOverlayX", webOverlayX);
  settings.setValue("webOverlayY", webOverlayY);
  settings.setValue("snsOverlayScale", snsOverlayScale);
  settings.setValue("snsOverlayX", snsOverlayX);
  settings.setValue("snsOverlayY", snsOverlayY);
  settings.setValue("cropSize", cropSize);
  settings.saveFile();
}

void ofApp::onTextVisitorNumberEvent(ofxDatGuiTextInputEvent e)
{
  if (e.text != "") {
    visitorNumber = ofToInt(e.text);
  } else {
    visitorNumber = ofApp::getNewVisitorNumber();
    textVisitorNumber->setText(ofToString(visitorNumber));
  }
}

void ofApp::onSliderPreviewFramerateEvent(ofxDatGuiSliderEvent e)
{
  ofSetFrameRate(previewFramerate);
}

void ofApp::onButtonReloadEvent(ofxDatGuiButtonEvent e)
{
  importTargets();
}

void ofApp::onColorPickerEvent(ofxDatGuiColorPickerEvent e)
{
  chromaKey.keyColor = e.color;
}

void ofApp::onPadWebPosition(ofxDatGui2dPadEvent e)
{
  webOverlayX = e.x;
  webOverlayY = e.y;
}

void ofApp::onPadSnsPosition(ofxDatGui2dPadEvent e)
{
  snsOverlayX = e.x;
  snsOverlayY = e.y;
}

void ofApp::onButtonExportEvent(ofxDatGuiButtonEvent e)
{
  exportStart();
}

void ofApp::onButtonUploadEvent(ofxDatGuiButtonEvent e)
{
  uploadAll();
}

void ofApp::onButtonPrintQREvent(ofxDatGuiButtonEvent e)
{
  printLastQRcode();
}

void ofApp::importTargets()
{
  say("Images loading has started.");
  targetDirectory.allowExt("jpg");
  targetDirectory.listDir();
  vector <ofFile> reversed = targetDirectory.getFiles();
  if (reversed.size() < FRAME_NUM) {
    ofSystemAlertDialog("Error! It requires " + ofToString(FRAME_NUM) + " photos or more.");
    logWithTimestamp("Error: Target photos aren't enough.");

    return;
  }
  isTargetLoaded = false;
  targetImages.clear();
  reverse(reversed.begin(), reversed.end());
  for (int i = 0; i < FRAME_NUM; i++) {
    logWithTimestamp("Loading image #" + ofToString(i, 2, '0') + ": " + reversed[i].getAbsolutePath());  // Just Debug!!

    ofImage importing;
    ofImage cropped;
    importing.load(reversed[i].getAbsolutePath());
    cropped.cropFrom(importing, (importing.getWidth() - TARGET_WIDTH) / 2, (importing.getHeight() - TARGET_HEIGHT) / 2, cropSize, cropSize);
    targetImages.insert(targetImages.begin(), cropped);
  }
  // chromaKey.keyColor = targetImages[0].getColor(0, 0);
  // colorPicker->setColor(chromaKey.keyColor);
  sliderCurrentFrame->setMax(targetImages.size() - 1);
  sliderWebCaptureFrame->setMax(targetImages.size() - 1);
  isTargetLoaded = true;
  say("Images loading is completed.");
}

void ofApp::importWebBackground()
{
  webBackgroundImage.load(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_web/web.png");
  webCheckerImage.load(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_web/checker.png");
  isWebBackgroundLoaded = true;
}

void ofApp::importSnsBackgrounds()
{
  // ofDirectory snsBackgroundDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_sns");
  // snsBackgroundDirectory.allowExt("jpg");
  // snsBackgroundDirectory.listDir();
  // snsBackgroundImages.clear();
  // for (ofFile f : snsBackgroundDirectory.getFiles()) {
  //   ofImage importing;
  //   importing.load(f.getAbsolutePath());
  //   snsBackgroundImages.push_back(importing);
  // }
  snsBackgroundImage.load(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_sns/sns.bmp");
  isSnsBackgroundLoaded = true;
}

void ofApp::importAndroidBackgrounds()
{
  // ofDirectory androidBackgroundDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_android");
  // androidBackgroundDirectory.allowExt("jpg");
  // androidBackgroundDirectory.listDir();
  // androidBackgroundImages.clear();
  // for (ofFile f : androidBackgroundDirectory.getFiles()) {
  //   ofImage importing;
  //   importing.load(f.getAbsolutePath());
  //   androidBackgroundImages.push_back(importing);
  // }
  androidBackgroundImage.load(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/background_android/android.jpg");
  isAndroidBackgroundLoaded = true;
}

void ofApp::exportStart()
{
  if (isTargetLoaded) {
    isExporting  = true;
    currentFrame = 0;
    say("Images exporting has started.");
    sliderCurrentFrame->setBackgroundColor(ofColor(255, 0, 0));
    sliderCurrentFrame->setLabel("Exporting...");
    ofApp::exportForWeb();
  }
}

void ofApp::exportForWeb()
{
  fbo_web.readToPixels(pixels);
  exportWebImage.setFromPixels(pixels);
  exportWebImage.save(exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/main.png", OF_IMAGE_QUALITY_BEST);
}

void ofApp::exportForSns()
{
  fbo_sns.readToPixels(pixels);
  exportSnsImage.setFromPixels(pixels);
  if (currentFrame == 0) {
    exportSnsImage.save(exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/white.png", OF_IMAGE_QUALITY_BEST);
  }
  exportSnsImage.save(exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/sns_" + ofToString(currentFrame, 3, '0') + ".png", OF_IMAGE_QUALITY_BEST);
}

void ofApp::exportForAndroid()
{
  fbo_android.readToPixels(pixels);
  exportAndroidImage.setFromPixels(pixels);
  exportAndroidImage.save(exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/android_" + ofToString(currentFrame, 3, '0') + ".png", OF_IMAGE_QUALITY_BEST);
}

void ofApp::exportFinish()
{
  isExporting = false;
  sliderCurrentFrame->setBackgroundColor(ofColor(250, 250, 250));
  sliderCurrentFrame->setLabel("Current Frame");
  say("Images exporting is completed.");

  ofApp::convertSnsMovie();
  ofApp::convertAndroidMovie();
  ofApp::uploadAll();
  ofApp::printQRcode();
  ofApp::prepareNext();
}

void ofApp::convertSnsMovie()
{
  string path = exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName();
  logWithTimestamp(ofSystem("/usr/local/bin/ffmpeg -y -r " + ofToString(previewFramerate) + " -i " + path + "/sns_%03d.png -pix_fmt yuv420p " + path + "/main.mp4" +
                            " && echo Converting SNS mp4 was a success. || echo Error: Converting SNS mp4 was a failure."));
}

void ofApp::convertAndroidMovie()
{
  string path = exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName();
  logWithTimestamp(ofSystem("/usr/local/bin/ffmpeg -y -r " + ofToString(previewFramerate) + " -i " + path + "/android_%03d.png -c:v libx264 -pix_fmt yuv420p -vf scale=1440:1396 " + path + "/movie.mp4" +
                            " && echo Converting Android mp4 was a success. || echo Error: Converting Android mp4 was a failure."));
}

void ofApp::uploadAll()
{
  // Delete Temporary
  logWithTimestamp(ofSystem("rm -f " + exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/android_*.png" +
                            " && echo Deleting temporary Android PNG files was a success. || echo Error: Deleting temporary Android PNG files was a failure."));
  logWithTimestamp(ofSystem("rm -f " + exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/sns_*.png" +
                            " && echo Deleting temporary SNS PNG files was a success. || echo Error: Deleting temporary SNS PNG files was a failure."));
  logWithTimestamp(ofSystem("/usr/local/bin/s3cmd sync --force --recursive --acl-public --no-guess-mime-type --no-check-md5 --exclude='.DS_Store' " + exportDirectory.getAbsolutePath() + "/ s3://data.nina-xmas.com/" +
                            " && echo S3cmd syncing was a success. || echo Error: S3cmd syncing was a failure."));
  say("Movies uploading is completed.");
}

void ofApp::printQRcode()
{
  string url        = "http://nina-xmas.com/share.php?d=" + ofGetTimestampString("%Y%m%d") + "&n=" + ofToString(visitorNumber, 3, '0');
  string exportPath = exportDirectory.getAbsolutePath() + "/" + ofApp::getExportName();
  logWithTimestamp(ofSystem("/usr/local/bin/qrencode -o " + exportPath + "/qr.png -l M \"" + url + "\"" + " && echo Making QR code image was a success. || echo Error: Making QR code image was a failure."));
  logWithTimestamp(ofSystem("/usr/local/bin/convert -font TimesNewRomanI -pointsize 24 label:'http://nina-xmas.com/\n" + ofGetTimestampString("%e %b. %Y") + "  #" + ofToString(visitorNumber, 3, '0') + "' " + exportPath + "/url.png" +
                            " && echo Making URL image was a success. || echo Error: Making URL image was a failure."));
  logWithTimestamp(ofSystem("/usr/local/bin/convert -gravity center -append " + exportPath + "/qr.png " + exportPath + "/url.png " + exportPath + "/qr.png" +
                            " && echo Composing QR code was a success. || echo Error: Composing QR code was a failure."));
  logWithTimestamp(ofSystem("lpr -P Brother_QL_700 -o PageSize=DC17 -o media=DC17 " + exportPath + "/qr.png" + " && echo QR code printing was a success. || echo Error: QR code printing was a failure."));
  say("QR code printing is completed.");
}

void ofApp::printLastQRcode()
{
  string url        = "http://nina-xmas.com/share.php?d=" + ofGetTimestampString("%Y%m%d") + "&n=" + ofToString(visitorNumber - 1, 3, '0');
  string exportPath = exportDirectory.getAbsolutePath() + "/" + ofGetTimestampString("%Y%m%d") + "/" + ofToString(visitorNumber - 1, 3, '0');
  logWithTimestamp(ofSystem("/usr/local/bin/qrencode -o " + exportPath + "/qr.png -l M \"" + url + "\"" + " && echo Making QR code image was a success. || echo Error: Making QR code image was a failure."));
  logWithTimestamp(ofSystem("/usr/local/bin/convert -font TimesNewRomanI -pointsize 24 label:'http://nina-xmas.com/\n" + ofGetTimestampString("%e %b. %Y") + "  #" + ofToString(visitorNumber - 1, 3, '0') + "' " + exportPath +
                            "/url.png" +
                            " && echo Making URL image was a success. || echo Error: Making URL image was a failure."));
  logWithTimestamp(ofSystem("/usr/local/bin/convert -gravity center -append " + exportPath + "/qr.png " + exportPath + "/url.png " + exportPath + "/qr.png" +
                            " && echo Composing QR code was a success. || echo Error: Composing QR code was a failure."));
  logWithTimestamp(ofSystem("lpr -P Brother_QL_700 -o PageSize=DC17 -o media=DC17 " + exportPath + "/qr.png" + " && echo QR code printing was a success. || echo Error: QR code printing was a failure."));
  say("QR code printing is completed.");
}

void ofApp::prepareNext()
{
  ofDirectory backupDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/backup_import");
  string      dirPath = backupDirectory.getAbsolutePath() + "/" + ofApp::getExportName() + "/";
  logWithTimestamp(ofSystem("mkdir -p " + dirPath + " && mv -f " + targetDirectory.getAbsolutePath() + "/* " + dirPath + " && echo Archiving ex-Target photos was a success. || echo Error: Archiving ex-Target photos was a failure."));
  visitorNumber = ofApp::getNewVisitorNumber();
  textVisitorNumber->setText(ofToString(visitorNumber));
  logWithTimestamp(" ---------------------------------------> " + ofToString(visitorNumber, 3, '0'));
}

int ofApp::getNewVisitorNumber()
{
  ofDirectory todayDirectory(ofFilePath::getUserHomeDir() + "/Desktop/NinaRicci/export/" + ofGetTimestampString("%Y%m%d"));
  if (!todayDirectory.exists()) {
    visitorNumber = 1;

    return visitorNumber;
  }
  todayDirectory.listDir();
  int exVisitorNumber = 0;
  for (ofFile f : todayDirectory.getFiles()) {
    if (f.isDirectory()) {
      int int_number = ofToInt(f.getBaseName());
      if (exVisitorNumber + 1 != int_number) {
        visitorNumber = exVisitorNumber + 1;

        return visitorNumber;
      } else {
        exVisitorNumber = int_number;
      }
    }
  }
  visitorNumber = exVisitorNumber + 1;

  return visitorNumber;
}

string ofApp::getExportName()
{
  return ofGetTimestampString("%Y%m%d") + "/" + ofToString(visitorNumber, 3, '0');
}

void ofApp::say(string msg)
{
  ofSystem("say -v Victoria " + msg);
  logWithTimestamp(msg);
}

void ofApp::logWithTimestamp(string msg)
{
  ofLog() << ofGetTimestampString("%H:%M:%S") << " " << msg;
}
