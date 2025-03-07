Ensemble Based Methods
======================

The reservoir model for a greenfield is based on a range of subsurface inputs such as seismic
data, a geological concept, well logs, and fluid samples. Data from the subsurface are scarce and
associated with significant uncertainty, and the resulting reservoir model is quite uncertain.
Although uncertain - reservoir models are still the primary tools for predicting future behavior
and making decisions for a field.

When the field has been in production for some time, one can use observed data sampled from the
producing field to update the model. This process is commonly called *history matching* in the
petroleum industry; in this manual, we will use the term *model updating*. Before the model
updating process can start, you will need:


1. A reservoir model which has been *parameterized* with a parameter set :math:`\{x\}` 
   that can produce simulated response :math:`y`.

2. Observation data, :math:`d`, from the producing field.

From a superficial point of view, model updating goes like this:

1. Simulate the behavior of the field and assemble simulated data :math:`y`. 

2. Compare the simulated data :math:`y` with the observed data :math:`d`. 

3. Based on the misfit between :math:`y` and :math:`d` updated parameters 
   :math:`\{x'\}` are calculated.   

Model updating falls into the general category of *inverse problems* - i.e., we know the results
and want to determine the input parameters which reproduce these results. In the statistical
literature, this process is often called *conditioning*. The algorithms available in ERT are all
based on Bayes' theorem:

.. math::

   f(x|d) \propto f(d|x)f(x),

where :math:`f(x|d)` is the posterior probability distribution i.e., the probability of :math:`x`
given :math:`d`, :math:`f(x)` is the prior distribution, and :math:`f(d|x)` is called the
likelihood i.e. the probability of observing :math:`d` assuming the reservoir is characterized by
:math:`x`. A successful sampling of reservoir models from the posterior distribution will not
only provide models with better history match (which in turn should provide better predictions)
but also assess the associated uncertainty.


.. figure:: images/Bayes_theorem.PNG
   :scale: 70%

   Bayes theorem illustrated; a prior distribution in blue is conditioned to dynamic production data
   to give the posterior distribution in red. Production profiles associated with realizations
   sampled from the prior and posterior distributions,  illustrate that the simulated responses of
   the posterior match the observed data better and the associated uncertainty is reduced after
   conditioning.


Embrace the uncertainty
-----------------------

The primary purpose of the model updating process is to reduce the uncertainty in the description
of the reservoir. However, it is essential to remember that the goal is *not* to get rid of all
the uncertainty and find one true answer. There are two reasons for this:

1. The data used, when conditioning the model, are also uncertain. The accuracy of measurements
   of, e.g. water cut and GOR, is limited by the precision in the measurement apparatus and the
   allocation procedures. For 4D seismic data, the uncertainty is considerable.

2. The model updating process will take place in the abstract space spanned by
   the parameters :math:`\{x\}`. Unless you are working on a synthetic
   example the *real reservoir* is certainly not in this space.

The goal is to update the parameters :math:`\{x\}` such that the resulting simulations on average
agree with the observations.  The variability in the simulations should be of the same order of
magnitude as the uncertainty in the data. The assumption is then that if this model is used for
predictions, it will be unbiased and give a realistic estimate of the future uncertainty (see
illustration in figure :numref:`ensemble`).

.. _ensemble:
.. figure:: images/bpr.jpg
   :scale: 20%

   Ensemble plots before and after model updating, for one succesfull updating
   and one updating which has gone wrong.


All the plots show simulations pressure in a cell as a function of time, with measurements. Plots
(1) and (3) show simulations before the model updating (i.e. the *prior*) and plots (2) and (4)
show the plots after the update process (the *posterior*). The dashed vertical line is meant to
illustrate the change from history to prediction. 
 
The left case with plots (1) and (2) is a successful history matching project. The simulations
from the posterior distribution are centered around the observed values, and the spread, i.e.,
the uncertainty, is of the same order of magnitude as the observation uncertainty. From this
case, we can reasonably expect that predictions will be unbiased with a reasonable estimate of
the uncertainty. 

For the right-hand case shown in plots (3) and (4) the model updating has *not* been successful
and more work is required. In the posterior solution, the simulations are not centered around the
observed values. When the observed values from the historical period are not correctly
reproduced, there is no reason to assume that the predictions will be correct either.
Furthermore, the uncertainty in the posterior case (4) is much smaller than the uncertainty in
the observations. This result does not make sense; although our goal is to reduce the
uncertainty, it should not be reduced significantly beyond the uncertainty of the observations.
The predictions from (4) will most probably be biased and significantly underestimate future
uncertainty [#]_.
 
.. [#] : (UNCLEAR) It should be emphasized that plots (3) and (4) show a simulated quantity from an
         assumed, more extensive set of observations. In general, there has been a different set of
         observations which has induced these large and unwanted updates.


Ensemble Kalman Filter - EnKF
-----------------------------
ERT was initially created to do model updating of reservoir models with the EnKF algorithm. The
experience from real-world models was that EnKF was not very suitable for reservoir applications.
Thus, ERT has since changed to use the Ensemble Smoother (ES), which can be said to be a
simplified version of the EnKF. But the characteristics of the EnKF algorithm still influence
many of the design decisions in ERT. It, therefore, makes sense to give a short introduction to
the Kalman Filter and EnKF.

The Kalman Filter
~~~~~~~~~~~~~~~~~


The Kalman Filter originates in electronics the 60's. The Kalman filter is *widely* used,
especially in applications where positioning is the goal, e.g., the GPS positioning. The typical
ingredients where the Kalman filter can be interesting to try include:

1. We want to determine the final *state* of the system - this can typically be the position.

2. The starting position is uncertain.

3. There is an *equation of motion* - or *forward model* - which describes how the system evolves
in time.

4. At a fixed point in time we can *observe* the system, these observations are uncertain.

As a straightforward application of the Kalman Filter, assume that we wish to estimate the
position of a boat as :math:`x(t)`. We know where the boat starts (initial condition), we have an
equation for how the boat moves in time, and at selected points in time :math:`t_k` we collect
*measurements* of the position. The quantities of interest are:

:math:`x_k`: The estimated position at time :math:`t_k`.

:math:`\sigma_k`: The uncertainty in the position at time :math:`t_k`.
   
:math:`x_k^{\ast}`: The *estimated/forecasted* position at time :math:`t_k`. 
   This is the position estimated from :math:`x_{k-1}` and :math:`g(x,t)`, but
   before the observed data :math:`d_k` are taken into account.

:math:`d_k`: The observed values that are used in the updating process. 
   The :math:`d_k` values are measured with a process external to the model updating.

:math:`\sigma_d`: The uncertainty in the measurements :math:`d_k`. 
      A reliable estimate of this uncertainty is essential for the algorithm to place a
      "correct" weight on the measured values.

:math:`g(x,t)`: The equation of motion - *forward model* - which propagates
       :math:`x_{k-1} \to x_k^{\ast}` 


The purpose of the Kalman Filter is to determine an updated :math:`x_k` from
:math:`x_{k-1}` and :math:`d_k`. The updated :math:`x_k` is the value that
*minimizes the variance* :math:`\sigma_k`. The equations for updated position
and uncertainty are:

.. math::

   x_k = x_k^{\ast}\frac{\sigma_d^2}{\sigma_k^2 + \sigma_d^2} + x_d
   \frac{\sigma_k^2}{\sigma_k^2 + \sigma_d^2}

.. math::          

   \sigma_k^2 = \sigma_k^i{2\ast}\left(1 - \frac{\sigma_k^{2\ast}}{\sigma_d^2 + \sigma_k^{2\ast}}\right)

In the equation for the position update, the analyzed position :math:`x_k` is a weighted sum over
the forecasted position :math:`x_k^{\ast}` and measured position :math:`d_k`.  The weighting
depends on the relative ratio of the uncertainties :math:`\sigma_k^{\ast}` and :math:`\sigma_d`.
For the updated uncertainty, the key takeaway message is that the updated uncertainty will always
be smaller than the forecasted uncertainty: :math:`\sigma_k < \sigma_k^{\ast}`.


Kalman Smoothers
------------------
We can derive the Kalman Filter updating equations starting from Bayes' theorem.  Assume
that we have a deterministic forward model, :math:`g(x)`, so that the predicted response
:math:`y` only depend on the model parameterized by the state vector :math:`x`

.. math::

   y = g(x).

In the classical history matching setting, :math:`x` represents the uncertainty 
parameters, :math:`g(x)` the forward model, and :math:`y` the simulated responses 
corresponding to the observed data, :math:`d`, from our oil field. From evaluating 
the model forward operator :math:`g(x)` of the uncertainty model parameters 
:math:`x \in \Re^n`, we determine a prediction :math:`y \in \Re^m`, which corresponds 
to the real measurements :math:`d \in \Re^m`. Here :math:`n` is the number of 
uncertainty parameters and :math:`m` is the number of observed measurements.

We introduce the mismatch :math:`e`
  
.. math::

   d = y + e.

We are interested in the posterior marginal distribution :math:`f(x|d)` which, according 
to Bayes theorem, can be expressed as  

.. math::

   f(x|d) \propto f(x)f(d|g(x)).

We introduce normal priors distributions

.. math::

   f(x) = \mathcal{N}(x^f,C_{xx}),

and assume that the data mismatch is normally distributed

.. math::

   f(d|g(x))=f(e)=\mathcal{N}(0,C_{dd}),

where :math:`x^f \in \Re^n` is the prior estimate of :math:`x` with covariance matrix 
:math:`C_{xx} \in \Re^{n \times n}`, and :math:`C_{dd} \in \Re^{m \times m}` is the 
error covariance for the measurements. We can then write the posterior distribution as

.. math::
   
   \begin{align}
   f(x|d) & \propto \exp\{-\frac{1}{2}(x-x^f)^T C_{xx}^{-1}(x-x^f)\} \\
          & \times \exp\{-\frac{1}{2}(g(x)-d)^T C_{dd}^{-1}(g(x)-d)\}.
   \end{align}

The smoother methods in ERT approximately sample the posterior PDF through various routes. 
These are derived exploiting the fact that maximizing f(x|d) is equivalent to minimizing

.. math::
   
   \begin{align}
   \mathcal{J}(x) & = -\frac{1}{2}(x-x^f)^T C_{xx}^{-1}(x-x^f) \\
          & + \frac{1}{2}(g(x)-d)^T C_{dd}^{-1}(g(x)-d).
   \end{align}

Solving :math:`\frac{\delta\mathcal{J(x)}}{\delta x} = 0`, using a linearization of :math:`g(x)`,
and using an averaged or best-fit model sensitivity represented by the linear regression

.. math::
   C_{xy} = GC_{xx},

where :math:`G = \nabla g(x)` yields


.. math::
   x = x^f + C_{xy}(C_{yy}^{f}+C_{dd})^{-1}(d_j-g(x_j^f)).

Thus, the update of :math:`x^f` is a linear and weighted correction, which in the linear case
would result in the minimum variance estimate.

 
Ensemble Smoother (ES)
----------------------
Ensemble methods attempt to sample the posterior Bayes's solution, by minimizing the ensemble of 
cost functions

.. math::
   
   \begin{align}
   \mathcal{J}(x_j) & = -\frac{1}{2}(x_j-x_j^f)^T C_{xx}^{-1}(x_j-x_j^f) \\
          & + \frac{1}{2}(g(x_j)-d_j)^T C_{dd}^{-1}(g(x_j)-d_j).
   \end{align}

Here probability distributions are represented by a collection of realizations, called an 
ensemble. Specifically, we introduce the prior ensemble

.. math::
   X^f = [x_1^f,\dots,x_n^f] = [x_j^f],

an :math:`n\times N` matrix sampled from the prior distribution. We also represent the data :math:`d` by an :math:`m\times N` matrix

.. math::
   D = [d_1,\dots,d_n] = [d_j],

so that the columns consist of the data vector plus a random vector from the normal distribution  

.. math::

   f(d|g(x))=f(e)=\mathcal{N}(0,C_{dd}).


The Ensemble Smoother algorithm approximately solves the minimization problems
:math:`\nabla\mathcal{J(x_j)}=0` for each realization.
To derive an equation for the updated :math:`x_j` that solves
:math:`\nabla\mathcal{J(x_j)}=0`, one must use the linearization:

.. math::
   g(x_j) = x_j^f + G_j(x_j -x_j^f)

where :math:`G_j = \nabla g(x_j)`.  The clever trick in ensemble methods is to replace the individual model sensitivities
:math:`G_j` by an ensemble averaged sensitivity :math:`G` represented by the linear regression equation

.. math::
   C_{yx} = G C_{xx}.

Covariances :math:`\bar{C}_{xy}`, :math:`\bar{C}_{xx}`, and :math:`\bar{C}_{dd}` are 
estimated from the ensemble and the state vector is updated according to:

.. math::
   \begin{align}
   x_j^a &= x_j^f + \bar{C}_{xy}(\bar{C}_{xy}^{f}\bar{C}_{xx}^{-1}\bar{C}_{xy}+\bar{C}_{dd})^{-1}(d_j-y_j^f)\\
   X^a &= X^f + \bar{C}_{xy}(\bar{C}_{xy}^{f}\bar{C}_{xx}^{-1}\bar{C}_{xy}+\bar{C}_{dd})^{-1}(D-Y_f).
   \end{align}

The model responses are then solved indireclty by evaluating the forward model

.. math::
   y_j^a = g(x_j^a).

The pseudo algorithm for ES:

1) Define :math:`D` by adding correlated noise according to :math:`C_{dd}`

2) Sample the prior ensemble, :math:`X_f`

3) Run the forward model :math:`Y_f = g(X_f)` to obtain the prior simulated responses

4) Calculate :math:`X_a` using equation above

5) Run the forward model :math:`Y_a = g(X_a)` to obtain the posterior simulated responses 


Numerical schemes
------------------
There are several nummerical schemes, i.e. methods to estimate the Kalman gain matrix

.. math::
   \bar{C}_{xy}(\bar{C}_{xy}^{f}\bar{C}_{xx}^{-1}\bar{C}_{xy}+\bar{C}_{dd})^{-1}

implemented in ERT. 


STD EnKF
~~~~~~~~

The recommended scheme. 


SQRT EnKF
~~~~~~~~~



NULL ENKF
~~~~~~~~~



FWD STEP EnKF
~~~~~~~~~~~~~



CV ENKF
~~~~~~~~



BOOTSTRAP ENKF
~~~~~~~~~~~~~~



Ensemble Smoother - Multiple Data Assimilation (ES MDA)
-------------------------------------------------------
While the Ensemble smoother attempts to solve the minimization equation in one go, the 
ES MDA iterates by introducing the observations gradually. The posterior distribution 
can be rewritten:

.. math::
   \begin{align}
   f(x|d) & \propto f(d|g(x))f(x)\\
          & \propto f(d|y)^{\frac{1}{\alpha_N}} \dots f(d|y)^{\frac{1}{\alpha_2}}f(d|y)^{\frac{1}{\alpha_1}}f(x) \\
	  & f(d|y)^{(\sum_{i=1}^N \frac{1}{\alpha_i})}f(x)
   \end{align}

with :math:`\sum_{i=1}^N \frac{1}{\alpha_i} = 1`.

In plain English, the ES MDA consist of several consecutive smoother updates with inflated 
error bars. The ES MDA with one iteration is identical to the Ensemble smoother. 


Iterative Ensemble Smoother - Ensemble subspace version
-------------------------------------------------------

The algorithm implemented is described in the article [Efficient Implementation of an Iterative Ensemble Smoother for Data Assimilation and Reservoir History Matching]( https://www.frontiersin.org/articles/10.3389/fams.2019.00047/full ).


Kalman Posterior Properties
---------------------------

The updating from the prior :math:`p(\psi)=N\left(\mu_\psi,\Sigma_\psi\right)` 
to the posterior :math:`p(\psi|d)=N\left(\mu_{\psi|d},\Sigma_{\psi|d}\right)`, 
in the process assimilating measurements :math:`d` that are linear in :math:`\psi`,
is performed by the Kalman methods by employing the following equations

.. math::
   \begin{align}
   \mu_{\psi|d} &= \mu_{\psi} + K(d-M\mu_{\psi}),\\
   \Sigma_{\psi|d} &= (I-KM) \Sigma_{\psi}
   \end{align}

where

.. math::
   \begin{align}
   K = \Sigma_{\psi}M^\top (M\Sigma_{\psi}M^\top + \Sigma_{d})^{-1}
   \end{align}

is called tha Kalman gain, and :math:`M` is the linear measurement operator (i.e., a matrix), so that

.. math::
   \begin{align}
   \hat{d} = M\mu_{\psi}
   \end{align}

is the best estimate of :math:`d` under the prior knowledge, and the error is assumed Gaussian with covariance :math:`\Sigma_d`. 
The ensemble variants draw an :math:`N`-sample :math:`\{\psi\}_{i=1}^N` from the prior, 
and perturb observations :math:`d` using the distributions of measurements creating a corresponding observation-sample :math:`\{d\}_{i=1}^N`.
The perturbations are guaranteed to sum to zero over the sample.
A posterior sample is then formed from updating the prior with the equation for the posterior mean above

.. math::
   \begin{align}
   \{\psi_i | d_i\} = \psi_i + \hat{K}(d_i-M \psi_i),
   \end{align}

where the estimated Kalman gain :math:`\hat{K}` is found by exchanging the prior covariance with an estimate based on its sample.
Thus, the ensemble methods combine a sample from the prior with a sample from the likelihood of observed data, to form a new sample from the posterior.
The posterior distribution that the posterior sample is conceptually sampled from, has mean and covariance found by

.. math::
   \begin{align}
   \hat{\mu}_{\psi|d} &= \bar{\psi} + \hat{K}(\bar{d}-M\bar{\psi}),\\
   \hat{\Sigma}_{\psi|d} &= (I-\hat{K}M) \hat{\Sigma}_{\psi}
   \end{align}

From this, it is seen that when the sample size tends to infinity and estimates converge to the corresponding population quantities,
then the ensemble variants converge to the standard Kalman filter in the linear Gaussian case.
This convergence is however of a stochastic nature.

More deterministic properties of the posterior are observed when the belief in measurements :math:`d` is varied. 
Intuitively, when measurements have zero belief, i.e. unbounded variance, then the posterior should equal the prior.
At the other end of the spectrum, if the measurements are perfect with zero variance,
then the posterior estimate should equal the maximum-likelihood estimate, corresponding to a flat prior, 
and as we are certain of the belief in this estimate (because the measurements are so amazing), 
the determinant of the posterior covariance tends to zero from above.
The maximum likelihood estimate is found by minimizing the relevant part of the negative log-likelihood of the data

.. math::
   \begin{align}
   \hat{\mu}_{ml} = \arg\min_{\mu} |d-M\mu|_2
   \end{align}

Furthermore, for a strictly decreasing sequence in belief in measurements, the distance between the
posterior and the maximum likelihood estimate will be strictly decreasing as well.
To summarize: 

- For the posterior estimate, we require that
  
  a. The information in :math:`d` has been assimilated, creating a better estimate, so that :math:`|\hat{\mu}_{\psi|d}-\hat{\mu}_{ml}|_2<|\hat{\mu}_{\psi}-\hat{\mu}_{ml}|_2` and :math:`|\hat{\mu}_{\psi|d}-\hat{\mu}_{\psi}|_2<|\hat{\mu}_{\psi}-\hat{\mu}_{ml}|_2`.
  b. The estimate improves at better quality data: Let :math:`\Sigma_d=\sigma_d I`. If a sequence of :math:`\sigma_d` decreases strictly, then so will the corresponding sequence of :math:`|\hat{\mu}_{\psi|d}-\hat{\mu}_{ml}|_2`.
  c. The estimate does not move from the prior at no information: When :math:`\sigma_d\to \infty` then :math:`|\hat{\mu}_{\psi|d}-\hat{\mu}_{\psi}|_2\to 0`.
  d. The estimate sequence converges to the ml-estimate:  When :math:`\sigma_d\to 0` then :math:`|\hat{\mu}_{\psi|d}-\hat{\mu}_{ml}|_2\to 0`.

- For the posterior covariance, we require for the `generalized variance <https://en.wikipedia.org/wiki/Generalized_variance>`_ that

  a. We become more certain of our estimates as informative data is assimilated, thus :math:`0<\det(\Sigma_{\psi|d})<\det(\Sigma_{\psi})`.
  b. We become increasingly certain in our estimates when increasingly informative data is assimilated: When a sequence of :math:`\sigma_d` decreases strictly, then so will the corresponding sequence of :math:`\det(\Sigma_{\psi|d})`.
  c. The certainty of our estimate does not move from the priors when assimilated data contains no information: When :math:`\sigma_d\to \infty` then :math:`\det(\Sigma_{\psi|d})\to\det(\Sigma_{\psi})` from below.
  d. If assimilated data is perfect, i.e., without noise, then we are fully certain of the posterior estiamte: When :math:`\sigma_d\to 0` then :math:`\det(\Sigma_{\psi|d})\to 0` from above.

In ert, the exact moments of the posterior are not calculated but can instead be estimated from the updated ensemble. 
The sample mean from the updated ensemble is guaranteed to equal the exact first moment of the posterior, due to the perturbations of :math:`d` summing to zero.
As a consequence, the maximum likelihood estimate is preserved.
This guarantees the path of the posterior estimate as below in Figure :numref:`posterior_path`.
Note however that this adjusts the sample slightly in both the case of measurements and posterior, but that this error is asymptotically negligible. 

.. _posterior_path:
.. figure:: images/posterior_path.png
   :scale: 100%

   Illustration of the deterministic path of the posterior estimate from 
   the priors to the likelihood estimate for :math:`\psi=[a,b]^\top`.
