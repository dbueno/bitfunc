# gen-timing-graphs.R
# This will go based on log data scrubbed from Klee and pre-processed
# It will assume that there will be only one set of timing information
# for each query (the pre-processing should make this so)

#cat ~/projs/Amphiboly/scripts/gen-timing-graphs.R | ./R --vanilla --setname inc ~/amphiboly-logs/stats/inc1.stats.preprocd ~/amphiboly-logs/stats/inc2.stats.preprocd --setname stp ~/amphiboly-logs/stats/stp1.stats.preprocd ~/amphiboly-logs/stats/stp2.stats.preprocd --yaxis QueryTime

# You can't interpolate or dodiff with a cactus plot or nominalx
# You can't cactusplot nominalx either

args <- commandArgs(); 		     # get the args

#print(args);

namearr <- vector();
setnames <- vector();
setargs <- list();
sctr <- 1;
files <- vector();
fctr <- 1;
setname <- "defset";
yname <- "QueryTime"
yaxis <- 2;
xname <- "NumQueries";
xaxis <- 1;
dodiff <- FALSE;
interpolate <- FALSE;
cactusplot <- FALSE;
nominalx <- FALSE;

case <- "parse";

for (n in 1:length(args)){
  curr <- args[n];

  if (substr(curr, 0, 2) == "--") {
#    print("Found next option");
    if (case == "filenames") {
      if (fctr > 1) {
        setnames[sctr] <- setname;
        setargs[[sctr]] <- files;	#unlist(files);
        sctr <- sctr+1;
        case <- "parse";
      } else {
        print ("Parsing error: no filenames provided in set ");
	print (setname);
        stop("Ending execution");
      }
    } else if (case == "yaxis") {
      print ("Parsing error: no yaxis number provided");
      stop("Ending execution");
    } else if (case == "xaxis") {
      print ("Parsing error: no xaxis number provided");
      stop("Ending execution");
    }
    if (case != "parse") {
      print ("Parsing error in case ");
      print (case);
      stop("Ending execution");
    }
  }

  if (case == "setname") {
    setname <- curr;
    files <- vector();
    fctr <- 1;
    case <- "filenames";
  } else if (case == "filenames") {
    files[fctr] <- curr;
    fctr <- fctr+1;
  } else if (case == "yaxis") {
    yname <- curr;			#as.real(curr)
    case <- "parse";
  } else if (case == "xaxis") {
    xname <- curr;			#as.real(curr)
    case <- "parse";
  } else {				# case should be "parse" here
    if (curr == "--setname") {
      case <- "setname";
    } else if (curr == "--yaxis") {
      case <- "yaxis";
    } else if (curr == "--xaxis") {
      case <- "xaxis";
    } else if (curr == "--dodiff") {
      dodiff <- TRUE;
    } else if (curr == "--interpolate") {
      interpolate <- TRUE;
    } else if (curr == "--cactus-plot") {
      cactusplot <- TRUE;
    } else if (curr == "--nominalx") {
      nominalx <- TRUE;
    } else {
      print("Unidentified option ");
      print(curr);
    }
  }
}

for (s in 1:length(setnames)) {
  print(setnames[s]);
  print(" consists of ");
  for (f in 1:length(setargs[s])) {
    print(setargs[s][f]);
  }
}

print("varname y-axis is ");
print(yname);

print("varname x-axis is ");
print(xname);

if (length(setnames) > 1) {
  palette(rainbow(length(setnames)));
}

fdata <- vector();
xdata <- list();
ydata <- list();
clrdata <- vector();
lndata <- vector();
ptdata <- vector();
dctr <- 1;

for (s in 1:length(setnames)){
  print(setargs[[s]]);
  lntypes <- c(1:length(setargs[[s]]));
  ptdata <- c(2:(length(setargs[[s]])+1));
  ptdata <- ptdata %% 19;
  for (f in 1:length(setargs[[s]])) {
    if (file.exists(setargs[[s]][f])) {
      tinfo <- read.table(setargs[[s]][f],TRUE,sep=",");
      yaxis <- match(yname, colnames(tinfo));
      if (is.na(yaxis)) {
        print("Error: couldn't find named field in file:");
	print(yname);
	print(setargs[[s]][f]);
        stop("Ending execution");
      }

      fdata[dctr] <- basename(setargs[[s]][f]);
      ydata[[dctr]] <- tinfo[1:length(tinfo[,1]),yaxis];
      clrdata[dctr] <- s;
      lndata[dctr] <- lntypes[f];
      ptdata[dctr] <- ptdata[f];
      
      if (cactusplot) {
	xdata[[dctr]] <- 1:length(tinfo[,1]);
      } else {
        xaxis <- match(xname, colnames(tinfo));
        if (is.na(xaxis)) {
          print("Error: couldn't find named field in file:");
  	  print(xname);
	  print(setargs[[s]][f]);
          stop("Ending execution");
        }
        xdata[[dctr]] <- tinfo[1:length(tinfo[,1]),xaxis];
      }

      #ddup based on x
      ydata[[dctr]] <- ydata[[dctr]][!duplicated(xdata[[dctr]])];
      xdata[[dctr]] <- unique(xdata[[dctr]]);

      if (cactusplot) {
        ydata[[dctr]] <- sort(ydata[[dctr]]);
      } else if (nominalx) {
        #how to assign namearr the first time?
	if (length(namearr) == 0) {
          namearr <- xdata[[dctr]];
        }	

        if (!setequal(namearr,xdata[[dctr]])) {
          print("Error: some entries were missing or added to x axis from ");
  	  print(xname);
	  print(setargs[[s]][f]);
          stop("Ending execution");
        }
	xdata[[dctr]] <- 1:length(ydata[[dctr]]);
      } else {
        if (interpolate) {
          missingQueries <- xdata[[dctr]] - 
                            append(c(0),
                                   xdata[[dctr]][1:length(ydata[[dctr]]) -1]);
          missingQueries <- missingQueries > 1;
          # I know there must be better ways to do this, but let's finish it up
          if (!is.na(match(TRUE, missingQueries))) {
            newydata <- vector();
            newxdata <- vector();
            appdidx <- 1;
            for (j in 1:length(ydata[[dctr]])) {
              if (missingQueries[j] == TRUE) {
                # This should never be possible for j is 1
                # Step should always be > 1
                xbeg <- xdata[[dctr]][j-1];
                step <- xdata[[dctr]][j] - xbeg;
                ybeg <- ydata[[dctr]][j-1];
                ystep <- (ydata[[dctr]][j] - ybeg) / step;
                for (k in 1:(step-1)) {
                  newxdata[appdidx] <- xbeg + k;
                  newydata[appdidx] <- ybeg + (k*ystep);
                  appdidx <- appdidx +1;
                }
              }  
              newxdata[appdidx] <- xdata[[dctr]][j];
              newydata[appdidx] <- ydata[[dctr]][j]; 
              appdidx <- appdidx +1;
            }
            xdata[[dctr]] <- newxdata;
            ydata[[dctr]] <- newydata;
          }
        }

        if (dodiff) {
          ydata[[dctr]] <- ydata[[dctr]] - 
                           append(c(0), 
                                  ydata[[dctr]][1:length(ydata[[dctr]]) -1]);
        }
      }

      dctr <- dctr +1;
    }
  }
}

#print(xdata);
#print(ydata);
#print("xranges:");
#print(xmin);
#print(xmax);
#print("yranges:");
#print(ymin);
#print(ymax);

xrange <- range(xdata);
yrange <- range(ydata);

pdf("output.pdf");

if (nominalx) {
  #axes=FALSE
  plot(0,cex=0.0,pch=20,xlim=xrange,ylim=yrange,xaxt="n",ann=FALSE);
  axis(1, las=2, at=1:length(namearr), lab=namearr);
} else {
  plot(0,cex=0.0,pch=20,xlim=xrange,ylim=yrange,ann=FALSE);
}

box();

print(ptdata);

for (i in 1:length(xdata)) {
  print(ptdata[[i]]);   
  lines(x=xdata[[i]],y=ydata[[i]],type="o",pch=ptdata[[i]],
        col=clrdata[[i]],lty=lndata[[i]]);
}

legend("topleft", 
       append(paste("Set",setnames),paste("   ",fdata)),
       col=append(1:length(setnames),clrdata),
       lty=append(rep(1,length(setnames)),lndata),
       pch=append(rep(46,length(setnames)),ptdata));
title(xlab=xname);
title(ylab=yname);
title(main=paste(xname,"vs.",yname,sep=" "), font.main=4);

dev.off();

# Save the plot as a pdf
