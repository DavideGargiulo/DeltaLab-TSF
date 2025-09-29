\NeedsTeXFormat{LaTeX2e}

\DeclareOption*{\PassOptionsToClass{\CurrentOption}{book}}
\ProcessOptions\relax
\LoadClass{book}

\def\fileversion{1.1}
\def\filedate{30/08/2023}

\RequirePackage{ifthen}

\RequirePackage[english]{babel}
\RequirePackage[T1]{fontenc}

\RequirePackage{geometry}
\RequirePackage{calc}

\RequirePackage{amsmath}
\RequirePackage{mathtools}
\RequirePackage{amssymb}

\RequirePackage{emptypage}

\RequirePackage{amsthm}
\RequirePackage{thmtools}
\RequirePackage[svgnames,table]{xcolor}
\RequirePackage{array}

\RequirePackage{graphicx}
\RequirePackage{setspace}
\RequirePackage{enumitem}
\RequirePackage{float}

\RequirePackage{titling}
\RequirePackage{titletoc}
\RequirePackage{etoc}
\RequirePackage[explicit]{titlesec}
\RequirePackage{eso-pic,transparent}

\RequirePackage{extramarks}
\RequirePackage{etoolbox}
\RequirePackage{fancyhdr}
\RequirePackage[type={CC},modifier={by-nc-nd},hyperxmp={false},version={4.0}]{doclicense}

\RequirePackage{xpatch}
\RequirePackage{rotating}
\RequirePackage{fbox}

% Some colors
\definecolor{arsenic}{rgb}{0.40, 0.40, 0.40}
\definecolor{airforceblue}{rgb}{0.36, 0.54, 0.66}
\definecolor{beaublue}{rgb}{0.74, 0.83, 0.9}
\definecolor{lightsilver}{RGB}{247,247,247}
\definecolor{silver}{RGB}{237,237,237}
\definecolor{darksilver}{RGB}{225,225,225}
\definecolor{ultralight}{RGB}{253, 253, 253}
\definecolor{codegreen}{rgb}{0,0.6,0}
\definecolor{codegray}{rgb}{0.5,0.5,0.5}
\definecolor{codepurple}{rgb}{0.58,0,0.82}
\definecolor{backcolour}{rgb}{0.95,0.95,0.92}

\makeatletter
\def\@doctitle{}
\def\@docsubtitle{}
\def\@dockeywords{}
\def\@footext{}
\def\@faculty{}
\def\@university{}
\def\@publisher{}
\def\@ams{}
\def\@city{}
\newcommand{\doctitle}[1]{\def\@doctitle{#1}}
\newcommand{\docsubtitle}[1]{\def\@docsubtitle{#1}}
\newcommand{\dockeywords}[1]{\def\@dockeywords{#1}}
\newcommand{\footext}[1]{\def\@footext{#1}}
\makeatother

% Some commands that will be useful for the copyrightpage
\makeatletter
\newcommand{\copyrightpage}[5]{\def\@faculty{#1}\def\@university{#2}\def\@publisher{#3}\def\@ams{#4}\def\@city{#5}}
\makeatother

% Number equations within section and tag only references equations.
\numberwithin{equation}{section}
\mathtoolsset{showonlyrefs,showmanualtags}

% Different theorem environments with respective styles.
\newtheorem{theorem}{Teorema}[section]
\newtheorem{corollary}[theorem]{Corollary}
\newtheorem{lemma}[theorem]{Lemma}
\newtheorem{exercise}[theorem]{Exercise}

\theoremstyle{definition}
\newtheorem{definition}[theorem]{Definition}

\theoremstyle{remark}
\newtheorem{remark}[theorem]{Remark}
\newtheorem{example}[theorem]{Example}

 % To make the lists more aesthetic, gray tones.
\setlist{label=\textcolor{gray}{\textbullet},topsep=2pt,noitemsep}
\setenumerate{label=\textcolor{gray}{\textit{\arabic*}.},topsep=2pt,noitemsep}

% Keeping the gray tones along the document, on the header bar.
\renewcommand{\headrulewidth}{1pt}
\xpretocmd\headrule{\color{lightgray}}{}{\PatchFailed}
\renewcommand{\qedsymbol}{\textcolor{gray}{$\blacksquare$}}

% Geometry
\geometry{a4paper, left=19.1mm, right=19.1mm, top=25.4mm, bottom=25.4mm}
% Important, header and footer spacing
\def\mylength{3pt}
\setlength{\headheight}{12pt+\mylength}
\addtolength{\headsep}{10pt}
\addtolength{\footskip}{10pt}
\addtolength{\textheight}{-20pt-1ex-\mylength}

% Par indent modifications
\setlength{\parindent}{0pt}

% If you want to put a quote, it can appear in italic.
\AtBeginEnvironment{quote}{\itshape}

% A beautiful background for the titlepage.
\newcommand\BackgroundPic[1]{\put(0,0){\parbox[b][\paperheight]{\paperwidth}{\vfill\centering\includegraphics[width=\paperwidth,height=\paperheight,keepaspectratio]{#1}\vfill}}}
\AtBeginEnvironment{titlepage}{\AddToShipoutPictureBG*{\transparent{0.1}\BackgroundPic{fibonacci.pdf}}}

% Page numbering
\newcommand{\pagenumber}[1]{{\setlength{\fboxsep}{5pt}\fbox[#1,lcolor=beaublue,rcolor=beaublue,tcolor=beaublue,bcolor=beaublue]{\fontsize{10pt}{10pt}\selectfont{\textcolor{gray!70!black}{\textsc{\thepage}}}}}}

% Header configuration
\makeatletter

\fancypagestyle{singleside}{%
\fancyhf{}%
\lhead{\color{gray}\nouppercase{\textit{\lastleftxmark}}}%
\rhead{\color{black}\nouppercase{\textsc{\lastrightxmark}}}%
\rfoot{\pagenumber{rt}}%
\lfoot{\fontsize{10pt}{10pt}\selectfont{\textsc{\textcolor{airforceblue}{\@doctitle}\;\textcolor{lightgray}{|}\;\textcolor{gray!70!black}{\@docsubtitle}}}}%
\cfoot{\color{gray!70!black}\fontsize{10pt}{10pt}{\@footext}}
\renewcommand{\headrulewidth}{1pt}}%

\fancypagestyle{twoside}{%
\fancyhf{}%
\fancyhead[RE]{\textcolor{black}{\nouppercase{\leftmark}}}%
\fancyhead[LO]{\textcolor{black}{\nouppercase{\lastrightxmark}}}%
\fancyhead[RO]{\textcolor{gray}{\textit{\lastleftxmark}}}%
\fancyhead[LE]{\textcolor{gray}{\textbf{\textsc{\Roman{chapter}}}}}%
\fancyfoot[RE]{\pagenumber{rt}}%
\fancyfoot[RO]{\pagenumber{rt}}%
\fancyfoot[LE]{\fontsize{10pt}{10pt}\selectfont{\textcolor{airforceblue}{\textsc{\@doctitle}}}}%
\fancyfoot[LO]{\fontsize{10pt}{10pt}\selectfont{\textcolor{gray!70!black}{\textsc{\@docsubtitle}}}}%
\fancyfoot[CO,CE]{\color{gray!70!black}\fontsize{10pt}{10pt}{\@footext}}%
\fancyheadoffset[leh,roh]{1em}%
\renewcommand{\headrulewidth}{1pt}}

\fancypagestyle{auxsingle}{%
\fancyhf{}%
\chead{\nouppercase{\textsc{\leftmark}}}%
% \lfoot{\fontsize{10pt}{10pt}\selectfont{\textsc{\textcolor{airforceblue}{\@doctitle}\;\textcolor{lightgray}{|}\;\textcolor{gray!70!black}{\@docsubtitle}}}}%
\rfoot{\pagenumber{lt}}%
\renewcommand{\headrulewidth}{1pt}}%

\fancypagestyle{auxtwoside}{%
\fancyhf{}%
\fancyhead[CE,CO]{\nouppercase{\textsc{\leftmark}}}%
\fancyfoot[RE]{\fontsize{10pt}{10pt}\selectfont{\textcolor{airforceblue}{\textsc{\@doctitle}}}}%
\fancyfoot[LO]{\fontsize{10pt}{10pt}\selectfont{\textcolor{gray!70!black}{\textsc{\@docsubtitle}}}}%
\fancyfoot[LE]{\pagenumber{lt}}%
\fancyfoot[RO]{\pagenumber{rt}}%
\fancyheadoffset{0em}%
\renewcommand{\headrulewidth}{1pt}}%

\makeatother

% Greetings for the template author command (modify the auxiliar pagestyles).
\newboolean{greet}
\newcommand{\greetings}{\setboolean{greet}{true}}

% Now, if we load all-in-one with two side option, we will adjust the headers accordingly
\makeatletter
\if@twoside
  \fancypagestyle{copyright}{
  \fancyhf{}%
  \fancyfoot[LE,LO]{\pagenumber{lt}}%
  \fancyheadoffset{0em}%
  \renewcommand{\headrulewidth}{0pt}}%
  \fancypagestyle{plain}{%
  \fancyhf{}%
  \fancyfoot[LE]{\fontsize{10pt}{10pt}\selectfont{\textcolor{airforceblue}{\textsc{\@doctitle}}}}%
  \fancyfoot[LO]{\fontsize{10pt}{10pt}\selectfont{\textcolor{gray!70!black}{\textsc{\@docsubtitle}}}}%
  \fancyfoot[RE]{\pagenumber{rt}}%
  \fancyfoot[RO]{\pagenumber{rt}}%
  \renewcommand{\headrulewidth}{0pt}}
\else
  \fancypagestyle{copyright}{
  \fancyhf{}%
  \lfoot{\pagenumber{lt}}%
  \fancyheadoffset{0em}%
  \renewcommand{\headrulewidth}{0pt}}%
  \fancypagestyle{plain}{%
  \fancyhf{}%
  \lfoot{\fontsize{10pt}{10pt}\selectfont{\textsc{\textcolor{airforceblue}{\@doctitle}\;\textcolor{lightgray}{|}\;\textcolor{gray!70!black}{\@docsubtitle}}}}%
  \rfoot{\pagenumber{rt}}%
  \renewcommand{\headrulewidth}{0pt}}%
\fi
\makeatother

\makeatletter
\if@twoside
  \newcommand{\defaultsettings}{twoside}
  \newcommand{\auxsettings}{auxtwoside}
\else
  \newcommand{\defaultsettings}{singleside}
  \newcommand{\auxsettings}{auxsingle}
\fi
\pagestyle{\defaultsettings}
\makeatother

% Section and subsection marks.
\makeatletter
\renewcommand{\chaptermark}[1]{\markboth{#1}{}\extramarks{}{}}
\renewcommand{\sectionmark}[1]{\extramarks{\thesection}{#1}}
\renewcommand{\subsectionmark}[1]{\extramarks{\thesubsection}{#1}}
\makeatother

% Part
\makeatletter
\def\@part[#1]#2#3{%
  \ifnum \c@secnumdepth >-2\relax%
    \refstepcounter{part}%
    \addcontentsline{toc}{part}{\thepart\hspace{1em}#1}%
  \else%
    \addcontentsline{toc}{part}{#1}%
  \fi%
  \markboth{}{}%
  \vspace*{-10em}%
  {\interlinepenalty\@M%
    \normalfont%
    \ifnum \c@secnumdepth >-2\relax%
      \huge\bfseries\partname\nobreakspace\thepart%
      \par
      \vskip 20\p@
    \fi%
    \Huge \bfseries #2}\par%
    \vskip 10\p@%
    {\normalfont #3}\par%
    \vskip 20\p@%
    \localtableofcontents
    \@endpart}
\makeatother

\newcommand*{\chapterselection}[3]{\IfStrEq{#1}{ }{#3}{\IfInteger{#1}{#2}{#3}}}

% Chapter
\titleformat{\chapter}[display]{\itshape\Large\centering\color{gray}}{\centering\Huge\bfseries\sc\chapterselection{\thechapter}{\Roman{chapter}}{\thechapter}}{1ex}{\titleline{\color{beaublue}\titlerule[1pt]\vspace{1ex}}\textcolor{black}{#1}}
\titleformat{name=\chapter,numberless}[display]{\itshape\Large\centering\color{black}}{}{1ex}{#1}[\titleline{\color{beaublue}\titlerule[1pt]\vspace{1ex}}]
\titlespacing*{\chapter}{0pt}{-50pt}{40pt}
% Section
\titleformat{\section}[frame]
{\color{lightgray}\large\normalfont\mdseries\centering}{\filcenter\small\quad\textcolor{gray}{\textit{\thesection}}\quad}
{1ex}{\color{gray!50!black}\Large\bfseries\filcenter#1}[]
% Subsection
\titleformat{\subsection}[hang]{\normalfont\mdseries\centering}{\color{gray}\textit{\thesubsection}}{2ex}{\colorbox{silver}{\begin{tabular}[t]{@{\color{lightgray}\vrule width 2pt\hspace{3ex}}p{\dimexpr\textwidth-6em\relax}}\raggedright#1\end{tabular}}}
% Subsubsection
\titleformat{\subsubsection}[hang]{\normalfont\bfseries}{\color{gray}\mdseries\textit{\thesubsubsection}}{1em}{#1}[]

% Equation numbering
\setcounter{secnumdepth}{3}
\arrayrulecolor{beaublue}
\numberwithin{equation}{section}