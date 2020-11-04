# gen-timing-graphs.R
# This will go based on log data scrubbed and pre-processed
# It will assume that there will be only one set of timing information
# for each query (the pre-processing should make this so)

args <- commandArgs(); 		     # get the args

#print(args);

setnames <- vector();
setargs <- list();
sctr <- 1;
files <- vector();
fctr <- 1;
setname <- "defset";
yname <-"QueryTime"
yaxis <- 2;
dodiff <- "FALSE";

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
      }
    } else if (case == "yaxis") {
      print ("Parsing error: no yaxis number provided");
    }
    if (case != "parse") {
      print ("Parsing error in case ");
      print (case);
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
  } else {				# case should be "parse" here
    if (curr == "--setname") {
      case <- "setname";
    } else if (curr == "--yaxis") {
      case <- "yaxis";
    } else if (curr == "--dodiff") {
      dodiff <- TRUE;
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

print("varname axis is ");
print(yname);

palette(rainbow(length(setnames)));

fdata <- vector();
xdata <- list();
ydata <- list();
clrdata <- vector();
lndata <- vector();
dctr <- 1;

xmin <- Inf;
ymin <- Inf;
xmax <- 0;
ymax <- 0;

for (s in 1:length(setnames)){
  print(setargs[[s]]);
  lntypes <- c(1:length(setargs[[s]]));
  for (f in 1:length(setargs[[s]])) {
    if (file.exists(setargs[[s]][f])) {
      tinfo <- read.table(setargs[[s]][f],TRUE,sep=",");
      yaxis <- match(yname, colnames(tinfo));
      if (is.na(yaxis)) {
        print("Error: couldn't find named field in file:");
	print(yname);
	print(setargs[[s]][f]);
        break;
      }
      xaxis <- match("NumQueries", colnames(tinfo));

      fdata[dctr] <- basename(setargs[[s]][f]);
      xdata[[dctr]] <- tinfo[1:length(tinfo[,1]),xaxis];
      ydata[[dctr]] <- tinfo[1:length(tinfo[,1]),yaxis];
      clrdata[dctr] <- s;
      lndata[dctr] <- lntypes[f];

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

      if (dodiff) {
        ydata[[dctr]] <- ydata[[dctr]] - 
                         append(c(0), 
                                ydata[[dctr]][1:length(ydata[[dctr]]) -1]);
      }

      if (min(xdata[[dctr]]) < xmin) {
        xmin <- min(xdata[[dctr]]);
      }
      if (min(ydata[[dctr]]) < ymin) {
        ymin <- min(ydata[[dctr]]);
      }
      if (max(xdata[[dctr]]) > xmax) {
        xmax <- max(xdata[[dctr]]);
      }
      if (max(ydata[[dctr]]) > ymax) {
        ymax <- max(ydata[[dctr]]);
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

pdf("output.pdf");
plot(0,cex=0.0,pch=20,xlim=c(xmin,xmax),ylim=c(ymin,ymax),
     xlab=paste("Query",sep=""), ylab=paste(yname,"(in s)"));
for (i in 1:length(xdata)) {
  lines(x=xdata[[i]],y=ydata[[i]],type="l",pch=20,
        col=clrdata[[i]],lty=lndata[[i]]);
}
legend("topleft", 
       append(paste("Set",setnames),paste("   ",fdata)),
       col=append(1:length(setnames),clrdata),
       lty=append(rep(1,length(setnames)),lndata));
dev.off();

# Save the plot as a pdf
